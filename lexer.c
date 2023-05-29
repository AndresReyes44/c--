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

    if(lex_is_in_expression())
    {
        buffer_write(lex_process->parentheses_buffer, c);
    }
    lex_process->pos.col += 1;                              // aumentamos la columna
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
    char c = peekc();
    LEX_GETC_IF(buffer, c, (c >= '0' && c <= '9'));

    buffer_write(buffer, 0x00);
    return buffer_ptr(buffer);
}
unsigned long long read_number()
{
    const char *s = read_number_str();
    return atoll(s);
}

struct token *token_make_number_for_value(unsigned long number)
{
    return token_create(&(struct token){.type = TOKEN_TYPE_NUMBER, .llnum = number});
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
    return op == '(' || op == '[' || op == ',' || op == '.' || op == '*';
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
           S_EQ(op, ",") ||
           S_EQ(op, ".");
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

struct token* token_make_multiline_comment()
{
    struct buffer* buffer = buffer_create();
    char c = 0;
    while(1)
    {
        LEX_GETC_IF(buffer, c, c != '*' && c != EOF);
        if (c == EOF)
        {
            compiler_error(lex_process->compiler, "Unexpected end comment\n");
        }
        else if(c == '*')
        {
            nextc();

            if(peekc() == '/')
            {
                nextc();
                break;
            }
        }
    }
    return token_create(&(struct token){.type = TOKEN_TYPE_COMMENT, .sval = buffer_ptr(buffer)});
}

struct token* handle_comment()
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
    if(is_keyword(buffer_ptr(buffer)))
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

struct token* token_make_newline()
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

char lexer_string_buffer_next_char(struct lex_process* process)
{
    struct buffer* buf = lex_process_private(process);
    return buffer_read(buf);
}
char lexer_string_buffer_peek_char(struct lex_process* process)
{
    struct buffer* buf = lex_process_private(process);
    return buffer_peek(buf);
}
void lexer_string_buffer_push_char(struct lex_process* process, char c)
{
    struct buffer* buf = lex_process_private(process);
    buffer_write(buf, c);
}
struct lex_process_functions lexer_string_bufffer_functions = {
    .next_char = lexer_string_buffer_next_char,
    .peek_char = lexer_string_buffer_peek_char,
    .push_char = lexer_string_buffer_push_char
};
struct lex_process* tokens_build_for_string(struct compile_process* compiler, const char* str)
{
    struct buffer* buffer = buffer_create();
    buffer_printf(buffer, str);
    struct lex_process* lex_process = lex_process_create(compiler, &lexer_string_bufffer_functions, buffer);
    if(!lex_process)
    {
        return NULL;
    } 

    if (lex(lex_process) != LEXICAL_ANALYSIS_ALL_OK)
    {
        return NULL;
    }
    return lex_process;   
}