#ifndef SCANNER_H
#define SCANNER_H

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define MAX_LEXEMA 256

// Enum para os tipos de átomos
typedef enum {
    T_PRG, T_VAR, T_SUBROT, T_INT, T_FLOAT, T_CHAR, T_VOID,
    T_READ, T_WRITE, T_IF, T_THEN, T_ELSE, T_FOR, T_WHILE,
    T_REPEAT, T_UNTIL, T_AND, T_OR, T_NOT, T_BEGIN, T_END,
    T_RETURN, T_ID, T_LITERAL_INT, T_LITERAL_FLOAT,
    T_LITERAL_CHAR, T_LITERAL_STRING, T_OP_ATRIB, T_OP_ARIT,
    T_OP_REL, T_OP_LOG, T_DELIM, T_ERRO, T_FIM
} TAtomo;

// Estrutura para armazenar o tipo do átomo, o lexema e a linha
typedef struct {
    TAtomo tipo;
    char lexema[MAX_LEXEMA];
    int linha;
} TInfoAtomo;

// Declaração da variável global do arquivo
extern FILE* arquivo;

// Protótipo da função principal
TInfoAtomo obter_atomo(void);

#endif