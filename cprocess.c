#include <stdio.h>
#include <stdlib.h>
#include "compiler.h"

//Definimos el proceso de compilacion
struct compile_process *compile_process_create(const char *filename, const char *filename_out, int flags)
{
    //Abrimos el archivo
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        return NULL; //si no se pudo abrir el archivo retorna null
    }

    //Abrimos el archivo de salida
    FILE *out_file = NULL;
    if (filename_out)
    {
        FILE *out_file = fopen(filename_out, "w");
        if (!out_file)
        {
            return NULL; //si no se pudo abrir el archivo de salida retorna null
        }
    }

    //asignamos process a la funcion calloc
    struct compile_process* process = calloc(1, sizeof(struct compile_process)); //la funcion calloc asigna memoria
    //process es un puntero a la estructura compile_process
    process->flags = flags; //asignamos los flags
    process->cfile.fp = file; //asignamos el archivo de entrada
    process->ofile = out_file; //asignamos el archivo de salida
    return process;
}


//Definimos la funcion que lee el siguiente caracter
char compile_process_next_char(struct lex_process* lex_process)
{
    struct compile_process* compiler = lex_process->compiler; //asignamos compiler a lex_process->compiler
    compiler->pos.col += 1; //aumentamos la columna
    char c = getc(compiler->cfile.fp); //leemos el siguiente caracter

    //si c es un salto de linea
    if (c == '\n') //si c es un salto de linea
    {
        compiler->pos.line += 1; //aumentamos la linea
        compiler->pos.col = 1; //reiniciamos la columna
    }

    return c; //regresa el caracter
}

//Permite ver el siguiente caracter y lo regresa para procesarlo despues
char compile_process_peek_char(struct lex_process* lex_process)
{
    struct compile_process* compiler = lex_process->compiler; //asignamos compiler a lex_process->compiler
    char c = getc(compiler->cfile.fp); //leemos el siguiente caracter
    ungetc(c, compiler->cfile.fp); //regresamos el caracter
    return c; //regresamos el caracter
}

//Permite regresar el caracter leido
void compile_process_push_char(struct lex_process* lex_process, char c)
{
    struct compile_process* compiler = lex_process->compiler; //asignamos compiler a lex_process->compiler
    ungetc(c, compiler->cfile.fp); //regresamos el caracter
}