#include <stdio.h>
#include "helpers/vector.h"
#include "compiler.h"

int main()
{
    // igualamos res a la funcion compile_file que lee el archivo
    int res = compile_file("./test.c", "./test", 0);
    if (res == COMPILER_FILE_COMPILED_OK)
    {
        printf("File compiled ok\n"); // si se compilo bien
    }
    else if (res == COMPILER_FAILED_WHIT_ERRORS)
    {
        printf("File compiled with errors\n"); // si se compilo con errores
    }
    else
    {
        printf("Unknown error\n"); // si hubo un error desconocido
    }

   

    return 0;
}
