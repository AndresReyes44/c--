#ifndef CompiladorFinal_H
#define CompiladorFinal_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define S_EQ(str, str2) \
    (str && str2 && (strcmp(str, str2)) == 0)

// Posicion en el archivo
struct pos
{
    int line;             // la linea en la que esta
    int col;              // la columna en la que esta
    const char *filename; // el nombre del archivo
};

// Definimos los numeros
#define NUMERIC_CASE \
    case '0':        \
    case '1':        \
    case '2':        \
    case '3':        \
    case '4':        \
    case '5':        \
    case '6':        \
    case '7':        \
    case '8':        \
    case '9':        \
    case '.'

#define OPERATOR_CASE_EXCLUDING_DIVISION \
    case '+':                            \
    case '-':                            \
    case '*':                            \
    case '=':                            \
    case '!':                            \
    case '<':                            \
    case '>':                            \
    case ',':                            \
    case '(':                            \
    case '['

#define SYMBOL_CASE \
    case '{':       \
    case '}':       \
    case ';':       \
    case ')':       \
    case ']'

// Definimos los errores DEL LEXER
enum
{
    LEXICAL_ANALYSIS_ALL_OK,
    LEXICAL_ANALYSIS_FAILED_INPUT_ERRORS
};
// Tipos de tokens
enum
{
    TOKEN_TYPE_IDENTIFIER,
    TOKEN_TYPE_KEYWORD,
    TOKEN_TYPE_OPERATOR,
    TOKEN_TYPE_SYMBOL,
    TOKEN_TYPE_NUMBER,
    TOKEN_TYPE_STRING,
    TOKEN_TYPE_COMMENT,
    TOKEN_TYPE_NEWLINE,
    TOKEN_TYPE_FLOAT
};

// Definimos como se comporta cada token
struct token
{
    int type;       // el tipo de token
    int flags;      // flags para el token
    struct pos pos; // la posicion en el archivo
    // valores que puede tener el token
    union
    {
        char cval;
        const char *sval;
        unsigned lnum;
        unsigned long long llnum;
        void *any;
        double dval;
    };

    // espacios en blancos entre tokens
    bool whitespace;

    // lo que esta entre brackets
    const char *between_brackets;
};

// Definimos como se comporta el lexer, funciona o no
enum
{
    COMPILER_FILE_COMPILED_OK,
    COMPILER_FAILED_WHIT_ERRORS
};

struct lex_process;
typedef char (*LEX_PROCESS_NEXT_CHAR)(struct lex_process *process);         // poder leer el siguiente caracter
typedef char (*LEX_PROCESS_PEEK_CHAR)(struct lex_process *process);         // poder ver el siguiente caracter
typedef void (*LEX_PROCESS_PUSH_CHAR)(struct lex_process *process, char c); // poder regresar un caracter

struct lex_process_functions
{
    LEX_PROCESS_NEXT_CHAR next_char;
    LEX_PROCESS_PEEK_CHAR peek_char;
    LEX_PROCESS_PUSH_CHAR push_char;
};

// estructura del lexer
struct lex_process
{
    struct pos pos;                   // la posicion en el archivo
    struct vector *token_vec;         // el vector de tokens
    struct compile_process *compiler; // el proceso de compilacion

    int current_expression_count;           // numero de brackets
    struct buffer *parentheses_buffer;      // buffer de los brackets
    struct lex_process_functions *function; // la funcion del lexer

    void *private; // estructura privada
};

typedef struct
{
    const char *type;
    const char *value;
} Token;

// Creamos el proceso de compilacion
struct compile_process
{
    // Los flags nos dicen como debe de ser compilado
    int flags;

    struct pos pos;
    // El archivo de entrada
    struct Compile_process_input_file
    {
        FILE *fp;
        const char *abs_path;
    } cfile;

    struct vector *token_vec; // vector de tokens
    // El archivo de salida
    FILE *ofile;
};

int compile_file(const char *filename, const char *output_filename, int flags);                            // compila un archivo
struct compile_process *compile_process_create(const char *filename, const char *filename_out, int flags); // crea un proceso de compilacion

// manipulacion de caracteres
char compile_process_next_char(struct lex_process *lex_process);
char compile_process_peek_char(struct lex_process *lex_process);
void compile_process_push_char(struct lex_process *lex_process, char c);

// mensajes de error y advertencia
void compiler_error(struct compile_process *compiler, const char *msg, ...);
void compiler_warning(struct compile_process *compiler, const char *msg, ...);

// inicio del lexer
struct lex_process *lex_process_create(struct compile_process *compiler, struct lex_process_functions *functions, void *private);
void lex_process_free(struct lex_process *process);
void *lex_process_private(struct lex_process *process);
struct vector *lex_process_tokens(struct lex_process *process);
int lex(struct lex_process *process);

// construccion de tokens para el archivo de entrada
struct lex_process *tokens_build_for_string(struct compile_process *compiler, const char *str);

// funciones para crear tokens
bool token_is_keyword(struct token *token, const char *value);
void printTokens(struct vector *token_vec);

#endif