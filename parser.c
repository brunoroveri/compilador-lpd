#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scanner.h"


static TInfoAtomo token_atual;


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

static void analisar_programa(void);
static void analisar_secao_var_opt(void);
static void analisar_decl_var(void);
static void analisar_tipo(void);

static void analisar_subrotinas_opt(void);
static void analisar_subrotina(void);
static void analisar_parametros_opt(void);

static void analisar_bloco(void);
static void analisar_lista_comandos(void);
static void analisar_comando(void);

static void analisar_atribuicao(void);
static void analisar_atribuicao_sem_pv(void);
static void analisar_if(void);
static void analisar_while(void);
static void analisar_for(void);
static void analisar_repeat(void);
static void analisar_read(void);
static void analisar_write(void);
static void analisar_return(void);


static void analisar_expressao(void); 
static void analisar_expressao_rel(void);      
static void analisar_expressao_arit(void);       
static void analisar_termo(void);                    
static void analisar_fator(void);                     


void iniciar_parser(FILE *fp) {
    arquivo = fp; 
    proximo();  
}



static void analisar_programa(void) {

    casar_token(T_PRG, NULL);
    casar_token(T_ID, NULL);
    casar_token(T_DELIM, ";");

    analisar_secao_var_opt();
    analisar_subrotinas_opt();

    analisar_bloco();
    casar_token(T_DELIM, ".");

    if (!token_e(T_FIM)) {
        erro_sintaxe("Tokens após término do programa", "EOF");
    }
}

static void analisar_subrotinas_opt(void) {
    while (token_e(T_SUBROT)) {
        analisar_subrotina();
    }
}

static void analisar_secao_var_opt(void) {
    if (token_e(T_VAR)) {
        casar_token(T_VAR, NULL);
        while (1) {
            analisar_decl_var();
            casar_token(T_DELIM, ";");
            if (token_e(T_ID) || token_e(T_INT) || token_e(T_FLOAT) || token_e(T_CHAR) || token_e(T_VOID))
                continue;
            break;
        }
        return;
    }

    while (token_e(T_ID) || token_e(T_INT) || token_e(T_FLOAT) || token_e(T_CHAR) || token_e(T_VOID)) {
        if (token_e(T_BEGIN) || token_e(T_SUBROT)) break;

        analisar_decl_var();
        casar_token(T_DELIM, ";");
    }
}

static void analisar_decl_var(void) {
    if (token_e(T_INT) || token_e(T_FLOAT) || token_e(T_CHAR) || token_e(T_VOID)) {
        analisar_tipo();                 
        casar_token(T_ID, NULL);     
        while (token_e_delim(",")) {    
            casar_token(T_DELIM, ",");
            casar_token(T_ID, NULL);
        }
        return;
    }

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

    erro_sintaxe("Declaração de variável inválida", "tipo id...  ou  id : tipo");
}

static void analisar_tipo(void) {
    if (token_e(T_INT) || token_e(T_FLOAT) || token_e(T_CHAR) || token_e(T_VOID)) {
        proximo();
    } else {
        erro_sintaxe("Tipo inválido", "int|float|char|void");
    }
}

static void analisar_subrotina(void) {
    casar_token(T_SUBROT, NULL);

    int cabecalho_tipo_first = 0;

    if (token_e(T_INT) || token_e(T_FLOAT) || token_e(T_CHAR) || token_e(T_VOID)) {
        analisar_tipo();
        cabecalho_tipo_first = 1;
        casar_token(T_ID, NULL);
    } else {
        casar_token(T_ID, NULL);
    }

    casar_token(T_DELIM, "(");
    analisar_parametros_opt();
    casar_token(T_DELIM, ")");

    if (!cabecalho_tipo_first && token_e_delim(":")) {
        casar_token(T_DELIM, ":");
        analisar_tipo();
    }

    if (token_e_delim(";")) {
        casar_token(T_DELIM, ";");
    }

    analisar_secao_var_opt();

    analisar_subrotinas_opt();

    analisar_bloco();

    if (token_e_delim(";")) {
        casar_token(T_DELIM, ";");
    }
}

static void analisar_parametros_opt(void) {
    /* vazio */
    if (token_e_delim(")")) return;

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


static void analisar_bloco(void) {
    casar_token(T_BEGIN, NULL);

    /* Declarações locais opcionais (formato tipo-first) */
    while (token_e(T_INT) || token_e(T_FLOAT) || token_e(T_CHAR) || token_e(T_VOID)) {
        analisar_decl_var();      /* já aceita:  tipo id (,id)*  */
        casar_token(T_DELIM, ";");
    }

    analisar_lista_comandos();
    casar_token(T_END, NULL);
}

static void analisar_lista_comandos(void) {
    while (1) {
        if (token_e(T_END)) break;

        analisar_comando();

        if (token_e_delim(";")) {
            casar_token(T_DELIM, ";");
            while (token_e_delim(";")) casar_token(T_DELIM, ";");
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
        analisar_atribuicao();
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
    casar_token(T_OP_ATRIB, NULL);
    analisar_expressao();
}

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

    if (token_e(T_ID)) {
        analisar_atribuicao_sem_pv();
    }
    casar_token(T_DELIM, ";");

    analisar_expressao();
    casar_token(T_DELIM, ";");

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

static void analisar_return(void) {
    casar_token(T_RETURN, NULL);
    if (!(token_e_delim(";"))) {
        analisar_expressao();
    }
}

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

static void analisar_fator(void) {
    if (token_e_delim("(")) {
        casar_token(T_DELIM, "(");
        analisar_expressao();
        casar_token(T_DELIM, ")");
        return;
    }
    if (token_e_op_log("not")) {
        proximo();
        analisar_fator();
        return;
    }
    if (token_e(T_ID)) {
        proximo();
        if (token_e_delim("(")) {
            casar_token(T_DELIM, "(");
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

void analisar_programa_public(void) {
    analisar_programa();
}