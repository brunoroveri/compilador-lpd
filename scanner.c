#include "scanner.h"

// Palavras reservadas da LPD
typedef struct {
    const char* palavra;
    TAtomo tipo;
} PalavraReservada;

PalavraReservada palavras_reservadas[] = {
    {"and", T_AND}, {"begin", T_BEGIN}, {"char", T_CHAR}, {"else", T_ELSE},
    {"end", T_END}, {"float", T_FLOAT}, {"for", T_FOR}, {"if", T_IF},
    {"int", T_INT}, {"not", T_NOT}, {"or", T_OR}, {"prg", T_PRG},
    {"read", T_READ}, {"repeat", T_REPEAT}, {"return", T_RETURN}, {"subrot", T_SUBROT},
    {"then", T_THEN}, {"void", T_VOID}, {"while", T_WHILE}, {"write", T_WRITE},
    {"until", T_UNTIL}, {"var", T_VAR},
    {NULL, T_ERRO}
};

FILE* arquivo;
int linha_atual = 1;

// Retorna o tipo de um identificador (palavra reservada ou ID)
TAtomo checar_palavra_reservada(const char* lexema) {
    for (int i = 0; palavras_reservadas[i].palavra != NULL; i++) {
        if (strcmp(lexema, palavras_reservadas[i].palavra) == 0) {
            return palavras_reservadas[i].tipo;
        }
    }
    return T_ID;
}

// Lida com a leitura de números (int e float)
TInfoAtomo ler_numero(char c) {
    TInfoAtomo atomo;
    int i = 0;
    int is_float = 0;

    atomo.linha = linha_atual;

    while (isdigit(c) || c == '.') {
        if (c == '.') {
            if (is_float) {
                atomo.tipo = T_ERRO;
                return atomo;
            }
            is_float = 1;
        }
        atomo.lexema[i++] = c;
        c = fgetc(arquivo);
    }

    ungetc(c, arquivo);
    atomo.lexema[i] = '\0';

    if (is_float) {
        atomo.tipo = T_LITERAL_FLOAT;
    } else {
        atomo.tipo = T_LITERAL_INT;
    }
    return atomo;
}

// Lida com a leitura de strings e caracteres
TInfoAtomo ler_literal(char delimitador) {
    TInfoAtomo atomo;
    int i = 0;
    char c = fgetc(arquivo);

    atomo.linha = linha_atual;

    while (c != delimitador && c != EOF) {
        if (i < MAX_LEXEMA - 1) {
            atomo.lexema[i++] = c;
        }
        c = fgetc(arquivo);
    }

    atomo.lexema[i] = '\0';

    if (c == EOF) {
        atomo.tipo = T_ERRO;
    } else if (delimitador == '"') {
        atomo.tipo = T_LITERAL_STRING;
    } else if (delimitador == '\'') {
        if (strlen(atomo.lexema) > 1) {
            atomo.tipo = T_ERRO;
        } else {
            atomo.tipo = T_LITERAL_CHAR;
        }
    } else {
         atomo.tipo = T_ERRO;
    }

    return atomo;
}

TInfoAtomo obter_atomo(void) {
    char c;
    TInfoAtomo atomo;
    int i = 0;

    while ((c = fgetc(arquivo)) != EOF) {
        if (c == '\n') {
            linha_atual++;
            continue;
        }
        if (isspace(c)) {
            continue;
        }
        if (c == '{') {
            while ((c = fgetc(arquivo)) != '}' && c != EOF) {
                if (c == '\n') linha_atual++;
            }
            if (c == EOF) {
                atomo.tipo = T_ERRO;
                return atomo;
            }
            continue;
        }

        if (isdigit(c)) {
            ungetc(c, arquivo);
            return ler_numero(fgetc(arquivo));
        }

        if (isalpha(c)) {
            i = 0;
            atomo.lexema[i++] = c;
            while (isalnum(c = fgetc(arquivo))) {
                atomo.lexema[i++] = c;
            }
            ungetc(c, arquivo);
            atomo.lexema[i] = '\0';
            atomo.tipo = checar_palavra_reservada(atomo.lexema);
            atomo.linha = linha_atual;
            return atomo;
        }

        if (c == '"' || c == '\'') {
            return ler_literal(c);
        }

        if (c == '<') {
            char next_c = fgetc(arquivo);
            if (next_c == '-') {
                strcpy(atomo.lexema, "<-");
                atomo.tipo = T_OP_ATRIB;
                atomo.linha = linha_atual;
                return atomo;
            }
            ungetc(next_c, arquivo);
        } else if (c == '=' || c == '!') {
            char next_c = fgetc(arquivo);
            if (next_c == '=') {
                atomo.lexema[0] = c;
                atomo.lexema[1] = '=';
                atomo.lexema[2] = '\0';
                atomo.tipo = T_OP_REL;
                atomo.linha = linha_atual;
                return atomo;
            }
            ungetc(next_c, arquivo);
        } else if (c == '>') {
            char next_c = fgetc(arquivo);
            if (next_c == '=') {
                strcpy(atomo.lexema, ">=");
                atomo.tipo = T_OP_REL;
                atomo.linha = linha_atual;
                return atomo;
            }
            ungetc(next_c, arquivo);
        }

        switch (c) {
            case '+': case '-': case '*': case '/':
                atomo.tipo = T_OP_ARIT;
                break;
            case '=': case '<': case '>':
                atomo.tipo = T_OP_REL;
                break;
            case '(': case ')': case '[': case ']': case ';': case ',': case '.':
                atomo.tipo = T_DELIM;
                break;
            default:
                atomo.tipo = T_ERRO;
                break;
        }

        atomo.lexema[0] = c;
        atomo.lexema[1] = '\0';
        atomo.linha = linha_atual;
        return atomo;
    }

    atomo.tipo = T_FIM;
    atomo.linha = linha_atual;
    return atomo;
}