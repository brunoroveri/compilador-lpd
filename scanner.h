#ifndef SCANNER_H
#define SCANNER_H

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define MAX_LEXEMA 256

// Enum para os tipos de átomos (tokens)
typedef enum {
    // Palavras-reservadas / tipos / estruturas
    T_PRG, T_VAR, T_SUBROT, T_INT, T_FLOAT, T_CHAR, T_VOID,
    T_READ, T_WRITE, T_IF, T_THEN, T_ELSE, T_FOR, T_WHILE,
    T_REPEAT, T_UNTIL, T_BEGIN, T_END, T_RETURN,

    // Identificadores e literais
    T_ID,
    T_LITERAL_INT, T_LITERAL_FLOAT, T_LITERAL_CHAR, T_LITERAL_STRING,

    // Operadores
    T_OP_ATRIB, // <-
    T_OP_ARIT,  // + - * /
    T_OP_REL,   // == != > < >= <=
    T_OP_LOG,   // and or not  (parser espera T_OP_LOG)

    // Delimitadores e controle
    T_DELIM,    // ( ) [ ] , ; .
    T_FIM,      // EOF
    T_ERRO
} TAtomo;

// Estrutura para token retornado pelo léxico
typedef struct {
    TAtomo tipo;
    char lexema[MAX_LEXEMA];
    int linha;
} TInfoAtomo;

// Arquivo fonte usado pelo léxico (definido em scanner.c)
extern FILE* arquivo;

// Função principal do analisador léxico
TInfoAtomo obter_atomo(void);

#endif