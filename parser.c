#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compiler.h"
#include "helpers/vector.h"
#include "helpers/buffer.h"

int getNextToken(int currentIndex) ;
void match(int expectedToken);

void declaration_list();
void declaration();
void var_declaration();
void var_declaration_prime();
void type_var();
void type_specifier();
void params();
void param_list();
void param_list_prime();
void param();
void param_prime();
void local_declarations();
void statement_list();
void statement();
void statement_prime();
void selection_stmt_prime();
void return_stmt();
void var();
void var_prime();
void expression();
void expression_prime();
void relop();
void arithmetic_expression();
void arithmetic_expression_prime();
void addop();
void term();
void term_prime();
void mulop();
void factor();
void factor_prime();
void args();
void arg_list();
void arg_list_prime();

int currentToken = 1;
int *tokens;
int cuenta = 0;

int getNextToken(int currentIndex) {
    if (currentIndex < cuenta) {
        int nextToken = tokens[currentIndex];
        currentIndex++;  // Avanzar al siguiente token
        return nextToken;
    } else {
        // Aquí puedes manejar una condición de error si se intenta obtener un token más allá del final del arreglo
        printf("Error: No hay más tokens disponibles.\n");
        exit(1);
    }
}

void match(int expectedToken)
{
    if (currentToken == expectedToken)
    {
        // Avanzar al siguiente token
        currentToken = getNextToken(currentToken);
    }
    else
    {
        printf("Error: Unexpected token '%d' in match\n", currentToken);
        exit(1);
    }
}



void program(int intTokens[], int counts) {
    tokens = intTokens;
    cuenta = counts;
    declaration_list();
    match(9); // void
    match(31); // ID
    match(24); // (
    match(9); // void
    match(25); // )
    match(26); // {
    local_declarations();
    statement_list();
    match(11); // return
    match(28); // ;
    match(27); // }
}

void declaration_list() {
    declaration();
    declaration_list();
}

void declaration() {
    var_declaration();
    type_specifier();
    match(31); // ID
    match(24); // (
    params();
    match(25); // )
    match(26); // {
    local_declarations();
    statement_list();
    match(27); // }
}

void var_declaration() {
    type_var();
    match(31); // ID
    var_declaration_prime();
}

void var_declaration_prime() {
    if (currentToken == 28) // ;
        match(28); // ;
    else if (currentToken == 23) // [
    {
        match(23); // [
        match(32); // INTEGER
        match(30); // ]
        match(28); // ;
    }
}

void type_var() {
    if (currentToken == 1) // int
        match(1); // int
    else if (currentToken == 4) // float
        match(4); // float
    else if (currentToken == 7) // string
        match(7); // string
}

void type_specifier() {
    if (currentToken == 1) // int
        match(1); // int
    else if (currentToken == 4) // float
        match(4); // float
    else if (currentToken == 7) // string
        match(7); // string
    else if (currentToken == 9) // void
        match(9); // void
}

void params() {
    if (currentToken == 1 || currentToken == 4 || currentToken == 7 || currentToken == 9 || currentToken == 31)
        param_list();
    else if (currentToken == 9) // void
        match(9); // void
}

void param_list() {
    param();
    param_list_prime();
}

void param_list_prime() {
    if (currentToken == 22) // ,
    {
        match(22); // ,
        param();
        param_list_prime();
    }
}

void param() {
    type_var();
    match(31); // ID
    param_prime();
}

void param_prime() {
    if (currentToken == 23) // [
    {
        match(23); // [
        match(24); // ]
    }
}

void local_declarations() {
    if (currentToken == 1 || currentToken == 4 || currentToken == 7 || currentToken == 9)
    {
        var_declaration();
        local_declarations();
    }
}

void statement_list() {
    if (currentToken == 31 || currentToken == 32 || currentToken == 33 || currentToken == 34 || currentToken == 23 || currentToken == 24 || currentToken == 25 || currentToken == 26 || currentToken == 30)
    {
        statement();
        statement_list();
    }
}

void statement() {
    if (currentToken == 31) // ID
    {
        match(31); // ID
        statement_prime();
    }
    else if (currentToken == 33) // STRING
    {
        match(33); // STRING
        match(28); // ;
    }
    else if (currentToken == 26) // {
    {
        match(26); // {
        local_declarations();
        statement_list();
        match(27); // }
    }
    else if (currentToken == 2) // if
    {
        match(2); // if
        match(24); // (
        expression();
        match(25); // )
        statement();
        selection_stmt_prime();
    }
    else if (currentToken == 8) // while
    {
        match(8); // while
        match(24); // (
        expression();
        match(25); // )
        statement();
    }
    else if (currentToken == 11) // return
    {
        match(11); // return
        return_stmt();
    }
    else if (currentToken == 3) // read
    {
        match(3); // read
        var();
        match(28); // ;
    }
    else if (currentToken == 6) // write
    {
        match(6); // write
        expression();
        match(28); // ;
    }
}

void statement_prime() {
    if (currentToken == 15) // =
    {
        match(15); // =
        expression();
        match(28); // ;
    }
    else if (currentToken == 24) // (
    {
        match(24); // (
        args();
        match(25); // )
        match(28); // ;
    }
}

void selection_stmt_prime() {
    if (currentToken == 5) // else
    {
        match(5); // else
        statement();
    }
}

void return_stmt() {
    if (currentToken == 28) // ;
        match(28); // ;
    else
    {
        expression();
        match(28); // ;
    }
}

void var() {
    match(31); // ID
    var_prime();
}

void var_prime() {
    if (currentToken == 23) // [
    {
        match(23); // [
        arithmetic_expression();
        match(30); // ]
    }
}

void expression() {
    arithmetic_expression();
    expression_prime();
}

void expression_prime() {
    if (currentToken == 16 || currentToken == 17 || currentToken == 18 || currentToken == 19 || currentToken == 20 || currentToken == 21)
    {
        relop();
        arithmetic_expression();
        expression_prime();
    }
}

void relop() {
    if (currentToken == 17) // <=
        match(17); // <=
    else if (currentToken == 16) // <
        match(16); // <
    else if (currentToken == 18) // >
        match(18); // >
    else if (currentToken == 19) // >=
        match(19); // >=
    else if (currentToken == 20) // ==
        match(20); // ==
    else if (currentToken == 21) // !=
        match(21); // !=
}

void arithmetic_expression() {
    term();
    arithmetic_expression_prime();
}

void arithmetic_expression_prime() {
    if (currentToken == 12 || currentToken == 13)
    {
        addop();
        term();
        arithmetic_expression_prime();
    }
}

void addop() {
    if (currentToken == 12) // +
        match(12); // +
    else if (currentToken == 13) // -
        match(13); // -
}

void term() {
    factor();
    term_prime();
}

void term_prime() {
    if (currentToken == 14 || currentToken == 15)
    {
        mulop();
        factor();
        term_prime();
    }
}

void mulop() {
    if (currentToken == 14) // *
        match(14); // *
    else if (currentToken == 15) // /
        match(15); // /
}

void factor() {
    if (currentToken == 31) // ID
    {
        match(31); // ID
        factor_prime();
    }
    else if (currentToken == 32) // NUM
    {
        match(32); // NUM
    }
    else if (currentToken == 33) // STRING
    {
        match(33); // STRING
    }
    else if (currentToken == 24) // (
    {
        match(24); // (
        expression();
        match(25); // )
    }
}

void factor_prime() {
    if (currentToken == 24) // (
    {
        match(24); // (
        args();
        match(25); // )
    }
    else if (currentToken == 23) // [
    {
        match(23); // [
        arithmetic_expression();
        match(30); // ]
    }
}

void args() {
    if (currentToken == 31 || currentToken == 32 || currentToken == 33 || currentToken == 24)
        arg_list();
}

void arg_list() {
    expression();
    arg_list_prime();
}

void arg_list_prime() {
    if (currentToken == 22) // ,
    {
        match(22); // ,
        expression();
        arg_list_prime();
    }
}
