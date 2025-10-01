#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scanner.h"

/* ===========================================================
   PARSER para LPD (descida recursiva)
   - Compatível com scanner que retorna:
     * T_ID, T_LITERAL_INT, T_LITERAL_FLOAT, T_LITERAL_CHAR, T_LITERAL_STRING,
     * T_PRG, T_VAR, T_SUBROT, T_INT, T_FLOAT, T_CHAR, T_VOID,
     * T_READ, T_WRITE, T_IF, T_THEN, T_ELSE, T_FOR, T_WHILE,
     * T_REPEAT, T_UNTIL, T_BEGIN, T_END, T_RETURN,
     * T_OP_ATRIB ("<-"), T_OP_ARIT (+ - * /), T_OP_REL (== != < > <= >=),
     * T_OP_LOG (and or not),
     * T_DELIM ( ( ) [ ] , ; . : ),
     * T_FIM, T_ERRO
   - Precedência: not > * / > + - > relacionais > and/or
   - Correções:
     * for(init; cond; update): init/update SEM ';' interno
     * var: aceita 'id,id: tipo;' e também 'tipo id,id;'
     * topo: aceita declarações sem 'var' logo após prg ... ;
     * bloco: aceita declarações locais tipo-first no início do begin...end
     * subrotina: aceita cabeçalho tipo-first e params "tipo id"
   =========================================================== */

static TInfoAtomo token_atual;

/* ---------- Utilidades ---------- */

static void erro_sintaxe(const char *msg, const char *esperado) {
    fprintf(stderr, "[ERRO SINTÁTICO] Linha %d: %s", token_atual.linha, msg);
    if (esperado) fprintf(stderr, " (esperado: %s)", esperado);
    if (token_atual.tipo == T_ERRO) {
        fprintf(stderr, " [léxico: %s]\n", token_atual.lexema);
    } else {
        fprintf(stderr, " [encontrei: tipo=%d lex=\"%s\"]\n", token_atual.tipo, token_atual.lexema);
    }
    exit(2);
}

static void proximo(void) {
    token_atual = obter_atomo();
    if (token_atual.tipo == T_ERRO) {
        erro_sintaxe("Token léxico inválido", token_atual.lexema);
    }
}

static int token_e(TAtomo t) { return token_atual.tipo == t; }
static int token_e_delim(const char *lex) {
    return token_atual.tipo == T_DELIM && strcmp(token_atual.lexema, lex) == 0;
}
static int token_e_op_arit(const char *lex) {
    return token_atual.tipo == T_OP_ARIT && (lex ? strcmp(token_atual.lexema, lex)==0 : 1);
}
static int token_e_op_log(const char *lex) {
    return token_atual.tipo == T_OP_LOG && (lex ? strcmp(token_atual.lexema, lex)==0 : 1);
}

static void casar_token(TAtomo t, const char *lex /*pode ser NULL*/) {
    if (token_atual.tipo != t) erro_sintaxe("Token inesperado", NULL);
    if (lex && strcmp(token_atual.lexema, lex) != 0) {
        erro_sintaxe("Lexema inesperado", lex);
    }
    proximo();
}

/* ---------- Protótipos ---------- */
static void analisar_programa(void);
static void analisar_secao_var_opt(void);
static void analisar_decl_var(void);
static void analisar_tipo(void);

static void analisar_subrotinas_opt(void);
static void analisar_subrotina(void);
static void analisar_parametros_opt(void); /* nova */

static void analisar_bloco(void);
static void analisar_lista_comandos(void);
static void analisar_comando(void);

static void analisar_atribuicao(void);
static void analisar_atribuicao_sem_pv(void); /* usada no for(...) */
static void analisar_if(void);
static void analisar_while(void);
static void analisar_for(void);
static void analisar_repeat(void);
static void analisar_read(void);
static void analisar_write(void);
static void analisar_return(void);

/* Expressões (precedência) */
static void analisar_expressao(void);                 // nível lógico (and/or)
static void analisar_expressao_rel(void);             // relação (== != < > <= >=)
static void analisar_expressao_arit(void);            // + -
static void analisar_termo(void);                     // * /
static void analisar_fator(void);                     // primários e not (unário)

/* ---------- Inicialização ---------- */
void iniciar_parser(FILE *fp) {
    arquivo = fp;     /* definido em scanner.c */
    proximo();        /* lê primeiro token */
}

/* ---------- Implementação ---------- */

static void analisar_programa(void) {
    /* prg ID ; [var/decls ...] [subrot ...] begin ... end . */
    casar_token(T_PRG, NULL);
    casar_token(T_ID, NULL);
    casar_token(T_DELIM, ";");

    analisar_secao_var_opt();
    analisar_subrotinas_opt();

    analisar_bloco();
    casar_token(T_DELIM, ".");    /* ponto final do programa */

    if (!token_e(T_FIM)) {
        erro_sintaxe("Tokens após término do programa", "EOF");
    }
}

/* subrotinas opcionais logo após a seção de variáveis do programa */
static void analisar_subrotinas_opt(void) {
    /* Na LPD, toda subrotina começa com a palavra-chave 'subrot' */
    while (token_e(T_SUBROT)) {
        analisar_subrotina();
    }
}

/* var ...  (aceita também declarações sem 'var' no topo:  int x;  float y,z;  ou  x,y : int;) */
static void analisar_secao_var_opt(void) {
    /* Caso 1: seção explícita com 'var' */
    if (token_e(T_VAR)) {
        casar_token(T_VAR, NULL);
        while (1) {
            analisar_decl_var();      /* uma declaração */
            casar_token(T_DELIM, ";");
            if (token_e(T_ID) || token_e(T_INT) || token_e(T_FLOAT) || token_e(T_CHAR) || token_e(T_VOID))
                continue;
            break;
        }
        return;
    }

    /* Caso 2: declarações IMEDIATAS após 'prg ... ;' sem 'var' */
    while (token_e(T_ID) || token_e(T_INT) || token_e(T_FLOAT) || token_e(T_CHAR) || token_e(T_VOID)) {
        /* Pare se chegou no início do bloco/subrotina */
        if (token_e(T_BEGIN) || token_e(T_SUBROT)) break;

        analisar_decl_var();
        casar_token(T_DELIM, ";");
    }
}

/* Uma declaração de variável, aceitando:
   - id (, id)* : tipo
   - tipo id (, id)*
*/
static void analisar_decl_var(void) {
    if (token_e(T_INT) || token_e(T_FLOAT) || token_e(T_CHAR) || token_e(T_VOID)) {
        /* Forma:  tipo id (, id)*  */
        analisar_tipo();                  // consome o tipo primeiro
        casar_token(T_ID, NULL);          // primeiro identificador
        while (token_e_delim(",")) {      // ids adicionais
            casar_token(T_DELIM, ",");
            casar_token(T_ID, NULL);
        }
        return;
    }

    if (token_e(T_ID)) {
        /* Forma:  id (, id)* : tipo  */
        casar_token(T_ID, NULL);
        while (token_e_delim(",")) {
            casar_token(T_DELIM, ",");
            casar_token(T_ID, NULL);
        }
        casar_token(T_DELIM, ":");
        analisar_tipo();                  // tipo no fim
        return;
    }

    erro_sintaxe("Declaração de variável inválida", "tipo id...  ou  id : tipo");
}

/* Ex.: int | float | char | void */
static void analisar_tipo(void) {
    if (token_e(T_INT) || token_e(T_FLOAT) || token_e(T_CHAR) || token_e(T_VOID)) {
        proximo();
    } else {
        erro_sintaxe("Tipo inválido", "int|float|char|void");
    }
}

/* subrotinas:
   Aceita dois formatos de cabeçalho:

   1)  subrot <ID> ( <params_opt> ) [: <tipo>] ;
   2)  subrot <tipo> <ID> ( <params_opt> ) ;

   Onde <params_opt> pode ser:
     - vazio
     - lista "tipo id" separados por vírgula (ex.: int A, float B)
     - OU lista "id, id : tipo" (compatibilidade)
*/
static void analisar_subrotina(void) {
    casar_token(T_SUBROT, NULL);

    int cabecalho_tipo_first = 0;

    /* Lookahead: tipo-first? */
    if (token_e(T_INT) || token_e(T_FLOAT) || token_e(T_CHAR) || token_e(T_VOID)) {
        /* formato: subrot <tipo> <ID> (...) */
        analisar_tipo();
        cabecalho_tipo_first = 1;
        casar_token(T_ID, NULL);
    } else {
        /* formato: subrot <ID> (...) [: tipo]? */
        casar_token(T_ID, NULL);
    }

    /* parâmetros */
    casar_token(T_DELIM, "(");
    analisar_parametros_opt();   /* aceita "tipo id" ou "id,id: tipo" */
    casar_token(T_DELIM, ")");

    /* no formato ID-first pode vir ": tipo" para retorno */
    if (!cabecalho_tipo_first && token_e_delim(":")) {
        casar_token(T_DELIM, ":");
        analisar_tipo();
    }

    /* ';' após o cabeçalho é opcional */
    if (token_e_delim(";")) {
        casar_token(T_DELIM, ";");
    }

    /* var locais opcionais e também declarações topo sem 'var' */
    analisar_secao_var_opt();

    /* >>> NOVO: permitir subrotinas aninhadas antes do begin */
    analisar_subrotinas_opt();

    /* corpo */
    analisar_bloco();

    /* ';' após end de subrotina (se presente) */
    if (token_e_delim(";")) {
        casar_token(T_DELIM, ";");
    }
}

/* parâmetros opcionais dentro de (...)  :
   - vazio
   - "tipo id" ( , "tipo id")*
   - OU "id (, id)* : tipo"    (compatibilidade)
*/
static void analisar_parametros_opt(void) {
    /* vazio */
    if (token_e_delim(")")) return;

    /* caso 1: tipo-first →  tipo id ( , tipo id )* */
    if (token_e(T_INT) || token_e(T_FLOAT) || token_e(T_CHAR) || token_e(T_VOID)) {
        while (1) {
            analisar_tipo();
            casar_token(T_ID, NULL);
            if (token_e_delim(",")) {
                casar_token(T_DELIM, ",");
                continue;
            }
            break;
        }
        return;
    }

    /* caso 2: id-first →  id (, id)* : tipo */
    if (token_e(T_ID)) {
        casar_token(T_ID, NULL);
        while (token_e_delim(",")) {
            casar_token(T_DELIM, ",");
            casar_token(T_ID, NULL);
        }
        casar_token(T_DELIM, ":");
        analisar_tipo();
        return;
    }

    erro_sintaxe("Parâmetros inválidos", "tipo id  ou  id : tipo");
}

/* begin ... end
   Aceita declarações locais tipo-first no início do bloco:
   begin
     int a, b;
     float x;
     ...comandos...
   end
*/
static void analisar_bloco(void) {
    casar_token(T_BEGIN, NULL);

    /* Declarações locais opcionais (formato tipo-first) */
    while (token_e(T_INT) || token_e(T_FLOAT) || token_e(T_CHAR) || token_e(T_VOID)) {
        analisar_decl_var();      /* já aceita:  tipo id (,id)*  */
        casar_token(T_DELIM, ";");
    }

    /* Depois das declarações (se houver), vêm os comandos */
    analisar_lista_comandos();
    casar_token(T_END, NULL);
}

/* comandos separados por ';' quando simples; compostos não exigem ';' após eles */
static void analisar_lista_comandos(void) {
    while (1) {
        if (token_e(T_END)) break;

        analisar_comando();

        if (token_e_delim(";")) {
            casar_token(T_DELIM, ";");
            while (token_e_delim(";")) casar_token(T_DELIM, ";"); /* permite ;; opcionais */
        } else {
            if (token_e(T_END)) break;
            if (!(token_e(T_ID) || token_e(T_READ) || token_e(T_WRITE) || token_e(T_RETURN) ||
                  token_e(T_BEGIN) || token_e(T_IF) || token_e(T_WHILE) || token_e(T_FOR) ||
                  token_e(T_REPEAT))) {
                break;
            }
        }
    }
}

/* despacho por 1º token */
static void analisar_comando(void) {
    if (token_e(T_ID)) {
        analisar_atribuicao();        /* comando simples → ';' vem na lista */
    } else if (token_e(T_READ)) {
        analisar_read();
    } else if (token_e(T_WRITE)) {
        analisar_write();
    } else if (token_e(T_RETURN)) {
        analisar_return();
    } else if (token_e(T_BEGIN)) {
        analisar_bloco();
    } else if (token_e(T_IF)) {
        analisar_if();
    } else if (token_e(T_WHILE)) {
        analisar_while();
    } else if (token_e(T_FOR)) {
        analisar_for();
    } else if (token_e(T_REPEAT)) {
        analisar_repeat();
    } else {
        erro_sintaxe("Início de comando inválido", NULL);
    }
}

/* ID <- expressao */
static void analisar_atribuicao(void) {
    casar_token(T_ID, NULL);
    casar_token(T_OP_ATRIB, NULL); /* "<-" */
    analisar_expressao();
}

/* Versão SEM ';' (para usar no for(init; cond; update)) */
static void analisar_atribuicao_sem_pv(void) {
    casar_token(T_ID, NULL);
    casar_token(T_OP_ATRIB, NULL); /* "<-" */
    analisar_expressao();
}

/* if (expressao) then comando [ else comando ] */
static void analisar_if(void) {
    casar_token(T_IF, NULL);
    casar_token(T_DELIM, "(");
    analisar_expressao();
    casar_token(T_DELIM, ")");

    casar_token(T_THEN, NULL);
    analisar_comando();

    if (token_e(T_ELSE)) {
        casar_token(T_ELSE, NULL);
        analisar_comando();
    }
}

/* while (expressao) comando */
static void analisar_while(void) {
    casar_token(T_WHILE, NULL);
    casar_token(T_DELIM, "(");
    analisar_expressao();
    casar_token(T_DELIM, ")");
    analisar_comando();
}

/* for ( init ; cond ; update ) comando  */
static void analisar_for(void) {
    casar_token(T_FOR, NULL);
    casar_token(T_DELIM, "(");

    /* init opcional: se começa com ID, é atribuição sem ';' */
    if (token_e(T_ID)) {
        analisar_atribuicao_sem_pv();
    }
    casar_token(T_DELIM, ";");

    /* cond: expressão (exigida) */
    analisar_expressao();
    casar_token(T_DELIM, ";");

    /* update opcional: se começa com ID, atribuição sem ';' */
    if (token_e(T_ID)) {
        analisar_atribuicao_sem_pv();
    }
    casar_token(T_DELIM, ")");

    analisar_comando();
}

/* repeat comando(s) until (expressao) */
static void analisar_repeat(void) {
    casar_token(T_REPEAT, NULL);
    if (token_e(T_BEGIN)) {
        analisar_bloco();
    } else {
        analisar_comando();
    }
    casar_token(T_UNTIL, NULL);
    casar_token(T_DELIM, "(");
    analisar_expressao();
    casar_token(T_DELIM, ")");
}

/* read( lista ) ;   onde lista = ID ( , ID )*  */
static void analisar_read(void) {
    casar_token(T_READ, NULL);
    casar_token(T_DELIM, "(");
    casar_token(T_ID, NULL);
    while (token_e_delim(",")) {
        casar_token(T_DELIM, ",");
        casar_token(T_ID, NULL);
    }
    casar_token(T_DELIM, ")");
}

/* write( lista ) ;   onde lista = (expressao | string | char) ( , ... )*  */
static void analisar_write(void) {
    casar_token(T_WRITE, NULL);
    casar_token(T_DELIM, "(");
    if (token_e(T_LITERAL_STRING) || token_e(T_LITERAL_CHAR)) {
        proximo();
    } else {
        analisar_expressao();
    }
    while (token_e_delim(",")) {
        casar_token(T_DELIM, ",");
        if (token_e(T_LITERAL_STRING) || token_e(T_LITERAL_CHAR)) {
            proximo();
        } else {
            analisar_expressao();
        }
    }
    casar_token(T_DELIM, ")");
}

/* return ;  |  return expressao ; */
static void analisar_return(void) {
    casar_token(T_RETURN, NULL);
    if (!(token_e_delim(";"))) {
        analisar_expressao();
    }
}

/* ----------------- EXPRESSÕES (precedência) ------------------ */
/* expressão_lógica ::= expressão_rel ( (and|or) expressão_rel )* */
static void analisar_expressao(void) {
    analisar_expressao_rel();
    while (token_e_op_log("and") || token_e_op_log("or")) {
        proximo();
        analisar_expressao_rel();
    }
}

/* expressão_rel ::= expressão_arit ( OP_REL expressão_arit )? */
static void analisar_expressao_rel(void) {
    analisar_expressao_arit();
    if (token_e(T_OP_REL)) {
        proximo(); /* == != < > <= >= */
        analisar_expressao_arit();
    }
}

/* expressão_arit ::= termo ( ('+'|'-') termo )* */
static void analisar_expressao_arit(void) {
    analisar_termo();
    while (token_e_op_arit("+") || token_e_op_arit("-")) {
        proximo();
        analisar_termo();
    }
}

/* termo ::= fator ( ('*'|'/') fator )* */
static void analisar_termo(void) {
    analisar_fator();
    while (token_e_op_arit("*") || token_e_op_arit("/")) {
        proximo();
        analisar_fator();
    }
}

/* fator ::= '(' expressão ')'
           |  'not' fator
           |  ID
           |  literal_int | literal_float | literal_char | literal_string
*/
static void analisar_fator(void) {
    if (token_e_delim("(")) {
        casar_token(T_DELIM, "(");
        analisar_expressao();
        casar_token(T_DELIM, ")");
        return;
    }
    if (token_e_op_log("not")) {
        proximo();
        analisar_fator(); /* not tem maior precedência */
        return;
    }
    if (token_e(T_ID)) {
        /* Pode ser apenas ID, ou chamada: ID '(' args? ')' */
        proximo(); /* consome o ID */
        if (token_e_delim("(")) {
            casar_token(T_DELIM, "(");
            /* args opcionais */
            if (!token_e_delim(")")) {
                analisar_expressao();
                while (token_e_delim(",")) {
                    casar_token(T_DELIM, ",");
                    analisar_expressao();
                }
            }
            casar_token(T_DELIM, ")");
        }
        return;
    }
    if (token_e(T_LITERAL_INT) || token_e(T_LITERAL_FLOAT) ||
        token_e(T_LITERAL_CHAR) || token_e(T_LITERAL_STRING)) {
        proximo();
        return;
    }

    erro_sintaxe("Fator inválido em expressão", NULL);
}

/* ===========================================================
   Entrada principal para analisar o programa completo.
   Use:
     iniciar_parser(arquivo);
     analisar_programa_public();
   =========================================================== */

void analisar_programa_public(void) {
    analisar_programa();
}