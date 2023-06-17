#include "compiler.h"
#include "helpers/vector.h"
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>


struct lex_process_functions compiler_lex_functions = {
    .next_char = compile_process_next_char,
    .peek_char = compile_process_peek_char,
    .push_char = compile_process_push_char,
};

// error de compilacion
void compiler_error(struct compile_process* compiler, const char* msg, ...)
{
    va_list args;                // lista de argumentos
    va_start(args, msg);         // inicializamos la lista de argumentos
    vfprintf(stderr, msg, args); // imprimimos el mensaje
    va_end(args);                // terminamos la lista de argumentos

    fprintf(stderr, "on line %i, col %i, in file %s\n", compiler->pos.line, compiler->pos.col, compiler->pos.filename); // mostramos la linea y la columna del error

    exit(-1); // salimos del programa
}
// advertencia de compilacion
void compiler_warning(struct compile_process* compiler, const char* msg, ...)
{
    va_list args;                // lista de argumentos
    va_start(args, msg);         // inicializamos la lista de argumentos
    vfprintf(stderr, msg, args); // imprimimos el mensaje
    va_end(args);                // terminamos la lista de argumentos

    fprintf(stderr, "on line %i, col %i, in file %s\n", compiler->pos.line, compiler->pos.col, compiler->pos.filename); // mostramos la linea y la columna de la advertencia
}

// Creamos el proceso de compilacion
int compile_file(const char* filename, const char* out_filename, int flags)
{
    struct compile_process* process = compile_process_create(filename, out_filename, flags);
    // Si no se pudo crear el proceso
    if (!process)
        return COMPILER_FAILED_WHIT_ERRORS;

    // LEXER
    struct lex_process* lex_process = lex_process_create(process, &compiler_lex_functions, NULL);
    // Si no se pudo crear el proceso
    if (!lex_process)
    {
        return COMPILER_FAILED_WHIT_ERRORS;
    }
    // si falla el analisis lexico
    if (lex(lex_process) != LEXICAL_ANALYSIS_ALL_OK)
    {
        return COMPILER_FAILED_WHIT_ERRORS;
    }

    process->token_vec = lex_process->token_vec; // asignamos el vector de tokens
    
    //mandamos a llamar el parser

    

   printTokens(lex_process->token_vec);

    return COMPILER_FILE_COMPILED_OK;
}

