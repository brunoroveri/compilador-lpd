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
    {NULL, T_ERRO} // Sentinela
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
                // Erro: mais de um ponto em um número
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
        atomo.tipo = T_ERRO; // String ou char não fechado
    } else if (delimitador == '"') {
        atomo.tipo = T_LITERAL_STRING;
    } else if (delimitador == '\'') {
        if (strlen(atomo.lexema) > 1) {
            atomo.tipo = T_ERRO; // Caractere literal com mais de um char
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
                atomo.tipo = T_ERRO; // Comentário não fechado
                return atomo;
            }
            continue;
        }

        // Números
        if (isdigit(c)) {
            ungetc(c, arquivo);
            return ler_numero(fgetc(arquivo));
        }

        // Identificadores e palavras reservadas
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

        // Literais (Strings e Chars)
        if (c == '"' || c == '\'') {
            return ler_literal(c);
        }

        // Operadores e delimitadores de múltiplos caracteres
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

        // Operadores e delimitadores de um único caractere
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

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s <nome_do_arquivo.lpd>\n", argv[0]);
        return -1;
    }

    arquivo = fopen(argv[1], "r");
    if (arquivo == NULL) {
        printf("Erro ao abrir o arquivo %s\n", argv[1]);
        return -1;
    }

    TInfoAtomo atomo;
    printf("Análise Léxica de %s\n\n", argv[1]);
    while ((atomo = obter_atomo()).tipo != T_FIM) {
        printf("Token: %d, Lexema: %s, Linha: %d\n", atomo.tipo, atomo.lexema, atomo.linha);
        if (atomo.tipo == T_ERRO) {
            printf("Erro: Caractere ou sequência de caracteres inválida\n");
            break;
        }
    }

    fclose(arquivo);
    return 0;
}