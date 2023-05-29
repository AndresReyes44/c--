#include "compiler.h"
#include "helpers/vector.h"
#include <stdlib.h>


//inicializamos todo lo que necesitamos para el lexer
struct lex_process* lex_process_create(struct compile_process* compiler, struct lex_process_functions* functions, void* private)
{
    struct lex_process* process = calloc(1, sizeof(struct lex_process)); //asignamos memoria y la inicializamos en 0
    process->function = functions; //asignamos la direccion de la funcion
    process->token_vec = vector_create(sizeof(struct token)); //creamos el vector de tokens encontrados
    process->compiler = compiler; //asignamos el proceso de compilacion
    process->private = private; //asignamos la estructura privada
    process->pos.line = 1; //inicializamos la linea
    process->pos.col = 1; //inicializamos la columna
    return process;
}

//liberamos el proceso
void lex_process_free(struct lex_process* process)
{
    vector_free(process->token_vec); //liberamos el vector de tokens
    free(process); //liberamos el proceso
}

//acceder a la estructura privada
void* lex_process_private(struct lex_process* process)
{
    return process->private; //regresamos la estructura privada
}

//acceder al vector de tokens
struct vector* lex_process_tokens(struct lex_process* process)
{
    return process->token_vec; //regresamos el vector de tokens
}