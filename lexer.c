#include "compiler.h"
#include "helpers/vector.h"
#include "helpers/buffer.h"
#include <string.h>
#include <assert.h>
#include <ctype.h>

// Leer caracteres mientras se cumpla las condiciones
#define LEX_GETC_IF(buffer, c, exp)     \
    for (c = peekc(); exp; c = peekc()) \
    {                                   \
        buffer_write(buffer, c);        \
        nextc();                        \
    }

struct token *read_next_token();
bool lex_is_in_expression();
// Definimos el proceso de compilacion estatico para que pueda ser accedido
static struct lex_process *lex_process;
// Definimos el token temporal
static struct token tmp_token;

static char peekc()
{
    return lex_process->function->peek_char(lex_process);
}

static char nextc()
{
    char c = lex_process->function->next_char(lex_process); // leemos el siguiente caracter

    if (lex_is_in_expression())
    {
        buffer_write(lex_process->parentheses_buffer, c);
    }
    lex_process->pos.col += 1; // aumentamos la columna
    // si c es un salto de linea
    if (c == '\n')
    {
        lex_process->pos.line += 1;
        lex_process->pos.col = 1;
    }

    return c;
}

static void pushc(char c)
{
    lex_process->function->push_char(lex_process, c);
}
static struct pos lex_file_position()
{
    return lex_process->pos;
}
struct token *token_create(struct token *_token)
{
    memcpy(&tmp_token, _token, sizeof(struct token));
    tmp_token.pos = lex_file_position();
    if (lex_is_in_expression())
    {
        tmp_token.between_brackets = buffer_ptr(lex_process->parentheses_buffer);
    }

    return &tmp_token;
}
static struct token *lexer_last_token()
{
    return vector_back_or_null(lex_process->token_vec);
}
static struct token *handle_whitespace()
{
    struct token *last_token = lexer_last_token();
    if (last_token)
    {
        last_token->whitespace = true;
    }
    nextc();
    return read_next_token();
}

const char *read_number_str()
{
    const char *num = NULL;
    struct buffer *buffer = buffer_create();
    char c = compile_process_peek_char(lex_process);

    if (c == '.') {
        // Si el primer caracter es un punto, no se considera un número válido
        compiler_error(lex_process->compiler, "Invalid symbol\n");
    }

    int num_chars = 0;
    while ((c >= '0' && c <= '9') || c == '.')
    {
        compile_process_next_char(lex_process);
        buffer_write(buffer, c);
        c = compile_process_peek_char(lex_process);
        num_chars++;
    }

    if (num_chars == 0 || buffer->data[num_chars-1] == '.') {
        // Si no se han leído caracteres o el último caracter es un punto, no es un número válido
        compiler_error(lex_process->compiler, "Invalid number\n");
        return NULL;
    }

    buffer_write(buffer, 0x00);
    return buffer_ptr(buffer);
}

double read_number()
{
    const char *s = read_number_str();
    return strtod(s, NULL);
}

struct token *token_make_number_for_value(double number)
{
    struct token *token = token_create(&(struct token){.type = TOKEN_TYPE_NUMBER});

    if (number - (unsigned long long)number == 0) {
        token->llnum = (unsigned long long)number;
    } else {
        token->type = TOKEN_TYPE_FLOAT; // Establecer el tipo como TOKEN_TYPE_FLOAT
        token->dval = number;
    }

    return token;
}

struct token *token_make_number()
{
    return token_make_number_for_value(read_number());
}

static struct token *token_make_string(char start_delim, char end_delim)
{
    struct buffer *buf = buffer_create();
    assert(nextc() == start_delim);
    char c = nextc();
    for (; c != end_delim && c != EOF; c = nextc())
    {
        if (c == '\\')
        {
            continue;
        }

        buffer_write(buf, c);
    }

    buffer_write(buf, 0x00);
    return token_create(&(struct token){.type = TOKEN_TYPE_STRING, .sval = buffer_ptr(buf)});
}

static bool op_treated_as_one(char op)
{
    return op == '(' || op == '[' || op == ',' || op == '*';
}

static bool is_single_operator(char op)
{
    return op == '+' || op == '-' || op == '/' || op == '!' || op == '=' || op == '<' || op == '>';
}
bool op_valid(const char *op)
{
    return S_EQ(op, "+") ||
           S_EQ(op, "-") ||
           S_EQ(op, "*") ||
           S_EQ(op, "/") ||
           S_EQ(op, "=") ||
           S_EQ(op, "==") ||
           S_EQ(op, "!=") ||
           S_EQ(op, "<") ||
           S_EQ(op, ">") ||
           S_EQ(op, "<=") ||
           S_EQ(op, ">=") ||
           S_EQ(op, "(") ||
           S_EQ(op, "[") ||
           S_EQ(op, ",");
}

void read_op_flush_back_keep(struct buffer *buffer)
{
    const char *data = buffer_ptr(buffer);
    int len = buffer->len;
    for (int i = len - 1; i >= 1; i--)
    {
        if (data[i] == 0x00)
        {
            continue;
        }

        pushc(data[i]);
    }
}
const char *read_op()
{
    bool single_operator = true;
    char op = nextc();
    struct buffer *buffer = buffer_create();
    buffer_write(buffer, op);

    if (!op_treated_as_one(op))
    {
        op = peekc();
        if (is_single_operator(op))
        {
            buffer_write(buffer, op);
            nextc();
            single_operator = false;
        }
    }

    buffer_write(buffer, 0x00);
    char *ptr = buffer_ptr(buffer);
    if (!single_operator)
    {
        if (!op_valid(ptr))
        {
            read_op_flush_back_keep(buffer);
            ptr[1] = 0x00;
        }
    }
    else if (!op_valid(ptr))
    {
        compiler_error(lex_process->compiler, "Unexpected operator %s \n", ptr);
    }

    return ptr;
}

static void lex_new_expression()
{
    lex_process->current_expression_count++;
    if (lex_process->current_expression_count == 1)
    {
        lex_process->parentheses_buffer = buffer_create();
    }
}

static void lex_finish_expression()
{
    lex_process->current_expression_count--;
    if (lex_process->current_expression_count < 0)
    {
        compiler_error(lex_process->compiler, "Unexpected closing bracket\n");
    }
}

bool lex_is_in_expression()
{
    return lex_process->current_expression_count > 0;
}

bool is_keyword(const char *str)
{
    return S_EQ(str, "int") ||
           S_EQ(str, "if") ||
           S_EQ(str, "read") ||
           S_EQ(str, "float") ||
           S_EQ(str, "else") ||
           S_EQ(str, "write") ||
           S_EQ(str, "string") ||
           S_EQ(str, "while") ||
           S_EQ(str, "void") ||
           S_EQ(str, "for") ||
           S_EQ(str, "return") ||
           S_EQ(str, "INT") ||
           S_EQ(str, "IF") ||
           S_EQ(str, "READ") ||
           S_EQ(str, "FLOAT") ||
           S_EQ(str, "ELSE") ||
           S_EQ(str, "WRITE") ||
           S_EQ(str, "STRING") ||
           S_EQ(str, "WHILE") ||
           S_EQ(str, "VOID") ||
           S_EQ(str, "FOR") ||
           S_EQ(str, "RETURN");
}

static struct token *token_make_operator_or_string()
{
    char op = peekc();

    struct token *token = token_create(&(struct token){.type = TOKEN_TYPE_OPERATOR, .sval = read_op()});
    if (op == '(')
    {
        lex_new_expression();
    }
    return token;
}

struct token *token_make_multiline_comment()
{
    struct buffer *buffer = buffer_create();
    char c = 0;
    while (1)
    {
        LEX_GETC_IF(buffer, c, c != '*' && c != EOF);
        if (c == EOF)
        {
            compiler_error(lex_process->compiler, "Unexpected end comment\n");
        }
        else if (c == '*')
        {
            nextc();

            if (peekc() == '/')
            {
                nextc();
                break;
            }
        }
    }
    return token_create(&(struct token){.type = TOKEN_TYPE_COMMENT, .sval = buffer_ptr(buffer)});
}

struct token *handle_comment()
{
    char c = peekc();
    if (c == '/')
    {
        nextc();
        if (peekc() == '*')
        {
            nextc();
            return token_make_multiline_comment();
        }

        pushc('/');
        return token_make_operator_or_string();
    }

    return NULL;
}

static struct token *token_make_symbol()
{
    char c = nextc();
    if (c == ')')
    {
        lex_finish_expression();
    }

    struct token *token = token_create(&(struct token){.type = TOKEN_TYPE_SYMBOL, .cval = c});
    return token;
}

static struct token *token_make_identifier_or_keyword()
{
    struct buffer *buffer = buffer_create();
    char c = 0;
    LEX_GETC_IF(buffer, c, (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'));

    buffer_write(buffer, 0x00);

    // keywords
    if (is_keyword(buffer_ptr(buffer)))
    {
        return token_create(&(struct token){.type = TOKEN_TYPE_KEYWORD, .sval = buffer_ptr(buffer)});
    }

    return token_create(&(struct token){.type = TOKEN_TYPE_IDENTIFIER, .sval = buffer_ptr(buffer)});
}

struct token *read_special_token()
{
    char c = peekc();
    if (isalpha(c))
    {
        return token_make_identifier_or_keyword();
    }

    return NULL;
}

struct token *token_make_newline()
{
    nextc();
    return token_create(&(struct token){.type = TOKEN_TYPE_NEWLINE});
}
struct token *read_next_token()
{
    struct token *token = NULL; // inicializamos el token
    char c = peekc();           // leemos el siguiente caracter

    token = handle_comment();
    if (token)
    {
        return token;
    }

    switch (c)
    {
    NUMERIC_CASE:
        token = token_make_number(); // creamos un token de tipo numero los casos numericos estan en el header
        break;

    OPERATOR_CASE_EXCLUDING_DIVISION:
        token = token_make_operator_or_string();
        break;

    SYMBOL_CASE:
        token = token_make_symbol();
        break;
    case '"':
        token = token_make_string('"', '"');
        break;
    // espaceos en blanco los consideramos pero no lo tomamos en cuenta
    case ' ':
    case '\t':
        token = handle_whitespace();
        break;

    case '\n':
        token = token_make_newline();
        break;
    case EOF:

        break;

    default:
        token = read_special_token();
        if (!token)
        {
            compiler_error(lex_process->compiler, "Unexpected character\n");
        }
    }
    return token;
}

int lex(struct lex_process *process)
{
    process->current_expression_count = 0;                     // inicializamos el numero de brackets
    process->parentheses_buffer = NULL;                        // inicializamos el buffer de brackets
    lex_process = process;                                     // asignamos el proceso de compilacion
    process->pos.filename = process->compiler->cfile.abs_path; // asignamos el nombre del archivo

    struct token *token = read_next_token();

    // mientras haya tokens
    while (token)
    {
        vector_push(process->token_vec, token); // agregamos el token al vector
        token = read_next_token();              // leemos el siguiente token
    }

    return LEXICAL_ANALYSIS_ALL_OK;
}

char lexer_string_buffer_next_char(struct lex_process *process)
{
    struct buffer *buf = lex_process_private(process);
    return buffer_read(buf);
}
char lexer_string_buffer_peek_char(struct lex_process *process)
{
    struct buffer *buf = lex_process_private(process);
    return buffer_peek(buf);
}
void lexer_string_buffer_push_char(struct lex_process *process, char c)
{
    struct buffer *buf = lex_process_private(process);
    buffer_write(buf, c);
}
struct lex_process_functions lexer_string_bufffer_functions = {
    .next_char = lexer_string_buffer_next_char,
    .peek_char = lexer_string_buffer_peek_char,
    .push_char = lexer_string_buffer_push_char};

struct lex_process *tokens_build_for_string(struct compile_process *compiler, const char *str)
{
    struct buffer *buffer = buffer_create();
    buffer_printf(buffer, str);
    struct lex_process *lex_process = lex_process_create(compiler, &lexer_string_bufffer_functions, buffer);
    if (!lex_process)
    {
        return NULL;
    }

    if (lex(lex_process) != LEXICAL_ANALYSIS_ALL_OK)
    {
        return NULL;
    }

    return lex_process;
}

typedef struct
{
    char *name;
    int count;
    int index;
} IdentifierEntry;

typedef struct
{
    IdentifierEntry *entries;
    int size;
    int capacity;
} IdentifierTable;

typedef struct
{
    char **types;
    int *counts;
    int *indices;
    int size;
    int capacity;
} NumberCounts;

int evaluateOperator(const char *operator)
{
    if (operator== NULL)
        return -1;

    if (strcmp(operator, "+") == 0)
        return 12;
    else if (strcmp(operator, "-") == 0)
        return 13;
    else if (strcmp(operator, "*") == 0)
        return 14;
    else if (strcmp(operator, "=") == 0)
        return 15;
    else if (strcmp(operator, "!") == 0)
        return 16;
    else if (strcmp(operator, "<") == 0)
        return 17;
    else if (strcmp(operator, ">") == 0)
        return 18;
    else if (strcmp(operator, ",") == 0)
        return 19;
    else if (strcmp(operator, ".") == 0)
        return 20;
    else if (strcmp(operator, "(") == 0)
        return 21;
    else if (strcmp(operator, "[") == 0)
        return 22;

    return -1;
}

int evaluateSymbol(char symbol)
{
    switch (symbol)
    {
    case '{':
        return 23;
    case '}':
        return 24;
    case ';':
        return 25;
    case ')':
        return 26;
    case ']':
        return 27;
    default:
        return 28;
    }
}

int evaluateKeyword(const char *keyword)
{
    if (strcmp(keyword, "int") == 0)
        return 1;
    else if (strcmp(keyword, "if") == 0)
        return 2;
    else if (strcmp(keyword, "read") == 0)
        return 3;
    else if (strcmp(keyword, "float") == 0)
        return 4;
    else if (strcmp(keyword, "else") == 0)
        return 5;
    else if (strcmp(keyword, "write") == 0)
        return 6;
    else if (strcmp(keyword, "string") == 0)
        return 7;
    else if (strcmp(keyword, "while") == 0)
        return 8;
    else if (strcmp(keyword, "void") == 0)
        return 9;
    else if (strcmp(keyword, "for") == 0)
        return 10;
    else if (strcmp(keyword, "return") == 0)
        return 11;
    else if (strcmp(keyword, "INT") == 0)
        return 1;
    else if (strcmp(keyword, "IF") == 0)
        return 2;
    else if (strcmp(keyword, "READ") == 0)
        return 3;
    else if (strcmp(keyword, "FLOAT") == 0)
        return 4;
    else if (strcmp(keyword, "ELSE") == 0)
        return 5;
    else if (strcmp(keyword, "WRITE") == 0)
        return 6;
    else if (strcmp(keyword, "STRING") == 0)
        return 7;
    else if (strcmp(keyword, "WHILE") == 0)
        return 8;
    else if (strcmp(keyword, "VOID") == 0)
        return 9;
    else if (strcmp(keyword, "FOR") == 0)
        return 10;
    else if (strcmp(keyword, "RETURN") == 0)
        return 11;
    else
        return -1;
}

void initializeIdentifierTable(IdentifierTable *table)
{
    table->size = 0;
    table->capacity = 10;
    table->entries = malloc(table->capacity * sizeof(IdentifierEntry));
}

void resizeIdentifierTable(IdentifierTable *table)
{
    table->capacity *= 2;
    table->entries = realloc(table->entries, table->capacity * sizeof(IdentifierEntry));
}

int findIdentifierIndex(IdentifierTable *table, const char *identifier)
{
    for (int i = 0; i < table->size; i++)
    {
        if (strcmp(table->entries[i].name, identifier) == 0)
        {
            return i;
        }
    }
    return -1;
}

int addIdentifier(IdentifierTable *table, const char *identifier)
{
    int index = findIdentifierIndex(table, identifier);
    if (index == -1)
    {
        if (table->size >= table->capacity)
        {
            resizeIdentifierTable(table);
        }

        table->entries[table->size].name = malloc((strlen(identifier) + 1) * sizeof(char));
        strcpy(table->entries[table->size].name, identifier);
        table->entries[table->size].count = 1;
        table->entries[table->size].index = table->size;
        table->size++;

        return table->size - 1;
    }
    else
    {
        return table->entries[index].index;
    }
}

void freeIdentifierTable(IdentifierTable *table)
{
    for (int i = 0; i < table->size; i++)
    {
        free(table->entries[i].name);
    }
    free(table->entries);
}

int sumIdentifierCounts(IdentifierTable *table)
{
    int sum = 0;
    for (int i = 0; i < table->size; i++)
    {
        sum += table->entries[i].count;
    }
    return sum;
}

void initializeNumberCounts(NumberCounts *counts)
{
    counts->size = 0;
    counts->capacity = 10;
    counts->types = malloc(counts->capacity * sizeof(char *));
    counts->counts = malloc(counts->capacity * sizeof(int));
    counts->indices = malloc(counts->capacity * sizeof(int));
}

void resizeNumberCounts(NumberCounts *counts)
{
    counts->capacity *= 2;
    counts->types = realloc(counts->types, counts->capacity * sizeof(char *));
    counts->counts = realloc(counts->counts, counts->capacity * sizeof(int));
    counts->indices = realloc(counts->indices, counts->capacity * sizeof(int));
}

int addNumber(NumberCounts *counts, const char *type)
{
    for (int i = 0; i < counts->size; i++)
    {
        if (strcmp(counts->types[i], type) == 0)
        {
            return counts->indices[i];
        }
    }

    if (counts->size >= 10)
    {
        return -1;
    }

    counts->types[counts->size] = malloc((strlen(type) + 1) * sizeof(char));
    strcpy(counts->types[counts->size], type);
    counts->counts[counts->size] = 1;
    counts->indices[counts->size] = counts->size;
    counts->size++;

    return counts->size - 1;
}

void freeNumberCounts(NumberCounts *counts)
{
    for (int i = 0; i < counts->size; i++)
    {
        free(counts->types[i]);
    }
    free(counts->types);
    free(counts->counts);
    free(counts->indices);
}

void printTokens(struct vector *token_vec)
{
    IdentifierTable identifierTable;
    initializeIdentifierTable(&identifierTable);
    NumberCounts numberCounts;
    initializeNumberCounts(&numberCounts);

    size_t numElements = vector_count(token_vec);

    for (size_t i = 0; i < numElements; i++)
    {
        struct token *token = (struct token *)vector_at(token_vec, i);

        int type = token->type;

        switch (type)
        {
        case TOKEN_TYPE_IDENTIFIER:
        {
            int count = addIdentifier(&identifierTable, token->sval);
            printf("<28,%d>\n", count);
            break;
        }
        case TOKEN_TYPE_KEYWORD:
        {
            int keywordValue = evaluateKeyword(token->sval);
            printf("<%d>\n", keywordValue);
            break;
        }
        case TOKEN_TYPE_OPERATOR:
        {
            int operatorValue = evaluateOperator(token->sval);
            printf("<%d>\n", operatorValue);
            break;
        }
        case TOKEN_TYPE_SYMBOL:
        {
            int symbolValue = evaluateSymbol(token->cval);
            printf("<%d>\n", symbolValue);
            break;
        }
        case TOKEN_TYPE_NUMBER:
        {
            char numStr[20];
            snprintf(numStr, sizeof(numStr), "%llu", token->llnum);
            int count = addNumber(&numberCounts, numStr);
            printf("<29,%d>\n", count);
            break;
        }
        case TOKEN_TYPE_STRING:
        {
            printf("<30>\n");
            break;
        }
        case TOKEN_TYPE_FLOAT:
        {
            char numStr[20];
            snprintf(numStr, sizeof(numStr), "%f", token->dval);
            int count = addNumber(&numberCounts, numStr);
            printf("<29,%d>\n", count);
            break;
        }
        case TOKEN_TYPE_COMMENT:
        case TOKEN_TYPE_NEWLINE:
            break;
        default:
        {
            printf("Unknown\n");
            break;
        }
        }
    }

    printf("Symbol Table for IDs\n");
    printf("Entry\t contents\n");
    for (int i = 0; i < identifierTable.size; i++)
    {
        printf("%d\t %s \n", i, identifierTable.entries[i].name);
    }

    printf("Symbol Table for numers\n");
    printf("Entry\t contents\n");
    for (int i = 0; i < numberCounts.size; i++)
    {
        printf("%d\t %s\n", i, numberCounts.types[i]);
    }

    freeIdentifierTable(&identifierTable);
    freeNumberCounts(&numberCounts);
}