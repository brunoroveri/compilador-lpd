#include "scanner.h"

/* Palavras reservadas da LPD */
typedef struct { const char* palavra; TAtomo tipo; } PalavraReservada;

static PalavraReservada palavras_reservadas[] = {
    {"and",    T_OP_LOG},   // operador lógico (parser espera T_OP_LOG)
    {"begin",  T_BEGIN},
    {"char",   T_CHAR},
    {"else",   T_ELSE},
    {"end",    T_END},
    {"float",  T_FLOAT},
    {"for",    T_FOR},
    {"if",     T_IF},
    {"int",    T_INT},
    {"not",    T_OP_LOG},   // operador lógico
    {"or",     T_OP_LOG},   // operador lógico
    {"prg",    T_PRG},
    {"read",   T_READ},
    {"repeat", T_REPEAT},
    {"return", T_RETURN},
    {"subrot", T_SUBROT},
    {"then",   T_THEN},
    {"until",  T_UNTIL},
    {"var",    T_VAR},
    {"void",   T_VOID},
    {"while",  T_WHILE},
    {"write",  T_WRITE},
    {NULL,     T_ERRO}
};

FILE* arquivo = NULL;
static int linha_atual = 1;

static int eh_letra(int c){ return (c=='_' || (c>='A'&&c<='Z') || (c>='a'&&c<='z')); }
static int eh_digito(int c){ return (c>='0' && c<='9'); }

static TAtomo checar_palavra_reservada(const char* lex){
    for (int i=0; palavras_reservadas[i].palavra != NULL; ++i){
        if (strcmp(lex, palavras_reservadas[i].palavra)==0)
            return palavras_reservadas[i].tipo;
    }
    return T_ID;
}

static void preencher(TInfoAtomo* a, TAtomo t, const char* lex){
    a->tipo = t;
    if (lex) {
        strncpy(a->lexema, lex, MAX_LEXEMA-1);
        a->lexema[MAX_LEXEMA-1] = '\0';
    } else {
        a->lexema[0]='\0';
    }
    a->linha = linha_atual;
}

static TInfoAtomo erro(const char* msg){
    TInfoAtomo a;
    preencher(&a, T_ERRO, msg);
    return a;
}

/* Lê string ou char literal */
static TInfoAtomo ler_literal(char delimitador) {
    TInfoAtomo atomo;
    int i = 0, c;
    atomo.linha = linha_atual;

    while ((c = fgetc(arquivo)) != EOF && c != delimitador) {
        if (delimitador=='"' && c=='\n') {
            return erro("String não pode quebrar linha");
        }
        if (i < MAX_LEXEMA - 1) atomo.lexema[i++] = (char)c;
    }

    if (c != delimitador) {
        return erro(delimitador=='"' ? "String não fechada" : "Char não fechado");
    }

    atomo.lexema[i] = '\0';
    if (delimitador=='"') {
        atomo.tipo = T_LITERAL_STRING;
    } else {
        if (strlen(atomo.lexema)!=1) return erro("Char deve ter 1 caractere");
        atomo.tipo = T_LITERAL_CHAR;
    }
    atomo.linha = linha_atual;
    return atomo;
}

TInfoAtomo obter_atomo(void) {
    TInfoAtomo atomo;
    int c;

    if (!arquivo) return erro("Arquivo não inicializado");

    /* Ignorar espaços, tabs, quebras e comentários { ... } */
    while ((c = fgetc(arquivo)) != EOF) {
        if (c == ' ' || c == '\t' || c == '\r') continue;
        else if (c == '\n') { linha_atual++; continue; }
        else if (c == '{') {
            int fechado = 0, d;
            while ((d = fgetc(arquivo)) != EOF) {
                if (d == '\n') linha_atual++;
                if (d == '}') { fechado = 1; break; }
            }
            if (!fechado) return erro("Comentário não fechado");
            continue;
        } else break;
    }

    if (c == EOF) { preencher(&atomo, T_FIM, NULL); return atomo; }

    /* Literais string/char */
    if (c == '"' || c == '\'') return ler_literal((char)c);

    /* Identificadores / Reservadas */
    if (eh_letra(c)) {
        char buffer[MAX_LEXEMA]; int i = 0;
        do {
            if (i < MAX_LEXEMA - 1) buffer[i++] = (char)c;
            c = fgetc(arquivo);
        } while (c != EOF && (eh_letra(c) || eh_digito(c)));
        if (c != EOF) ungetc(c, arquivo);
        buffer[i] = '\0';

        TAtomo t = checar_palavra_reservada(buffer);
        preencher(&atomo, t, buffer);
        return atomo;
    }

    /* Números (int ou float) */
    if (eh_digito(c)) {
        char buffer[MAX_LEXEMA]; int i = 0;
        do {
            if (i < MAX_LEXEMA - 1) buffer[i++] = (char)c;
            c = fgetc(arquivo);
        } while (c != EOF && eh_digito(c));

        if (c == '.') {
            int d = fgetc(arquivo);
            if (eh_digito(d)) {
                if (i < MAX_LEXEMA - 1) buffer[i++]='.';
                do {
                    if (i < MAX_LEXEMA - 1) buffer[i++] = (char)d;
                    d = fgetc(arquivo);
                } while (d != EOF && eh_digito(d));
                if (d != EOF) ungetc(d, arquivo);
                buffer[i] = '\0';
                preencher(&atomo, T_LITERAL_FLOAT, buffer);
                return atomo;
            } else {
                if (d != EOF) ungetc(d, arquivo);
                ungetc('.', arquivo);
            }
        } else {
            if (c != EOF) ungetc(c, arquivo);
        }

        buffer[i] = '\0';
        preencher(&atomo, T_LITERAL_INT, buffer);
        return atomo;
    }

    /* Operadores compostos e simples: <, >, =, !  */
    if (c == '<') {
        int next_c = fgetc(arquivo);
        if (next_c == '-') { preencher(&atomo, T_OP_ATRIB, "<-"); return atomo; }
        if (next_c == '=') { preencher(&atomo, T_OP_REL, "<=");  return atomo; }
        if (next_c != EOF) ungetc(next_c, arquivo);
        preencher(&atomo, T_OP_REL, "<"); return atomo;
    }

    if (c == '>') {
        int next_c = fgetc(arquivo);
        if (next_c == '=') { preencher(&atomo, T_OP_REL, ">="); return atomo; }
        if (next_c != EOF) ungetc(next_c, arquivo);
        preencher(&atomo, T_OP_REL, ">"); return atomo;
    }

    if (c == '=') {
        int next_c = fgetc(arquivo);
        if (next_c == '=') { preencher(&atomo, T_OP_REL, "=="); return atomo; }
        if (next_c != EOF) ungetc(next_c, arquivo);
        return erro("Use '==' para igualdade");
    }

    if (c == '!') {
        int next_c = fgetc(arquivo);
        if (next_c == '=') { preencher(&atomo, T_OP_REL, "!="); return atomo; }
        if (next_c != EOF) ungetc(next_c, arquivo);
        return erro("Use '!=' para diferente");
    }

    /* Operadores e comentários iniciados por '/' */
    if (c=='+' || c=='-' || c=='*' || c=='/') {
        if (c == '/') {
            int d = fgetc(arquivo);
            if (d == '/') {
                /* comentário de linha: // ... até \n ou EOF */
                while ((d = fgetc(arquivo)) != EOF && d != '\n') { /* skip */ }
                if (d == '\n') linha_atual++;
                return obter_atomo(); /* retoma leitura */
            } else if (d == '*') {
                /* comentário de bloco:  / * ... * / */
                int prev = 0, cur = 0, fechado = 0;
                while ((cur = fgetc(arquivo)) != EOF) {
                    if (cur == '\n') linha_atual++;
                    if (prev == '*' && cur == '/') { fechado = 1; break; }
                    prev = cur;
                }
                if (!fechado) return erro("Comentário /* */ não fechado");
                return obter_atomo(); /* retoma leitura */
            } else {
                if (d != EOF) ungetc(d, arquivo);
                /* não era comentário → é operador '/' */
            }
        }
        char op[2] = {(char)c, '\0'};
        preencher(&atomo, T_OP_ARIT, op);
        return atomo;
    }

    /* Delimitadores */
    switch (c) {
        case '(': preencher(&atomo, T_DELIM, "("); return atomo;
        case ')': preencher(&atomo, T_DELIM, ")"); return atomo;
        case '[': preencher(&atomo, T_DELIM, "["); return atomo;
        case ']': preencher(&atomo, T_DELIM, "]"); return atomo;
        case ',': preencher(&atomo, T_DELIM, ","); return atomo;
        case ';': preencher(&atomo, T_DELIM, ";"); return atomo;
        case '.': preencher(&atomo, T_DELIM, "."); return atomo;
        case ':': preencher(&atomo, T_DELIM, ":"); return atomo;
        default: break;
    }

    /* Caractere inválido */
    char msg[64];
    snprintf(msg, sizeof(msg), "Caractere inválido: '%c'", c);
    return erro(msg);
}