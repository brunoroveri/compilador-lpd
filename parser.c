#include "scanner.h"

// --- Protótipos das funções ---
void analisar_programa();
void analisar_secao_var();
void analisar_declaracao_variavel();
void analisar_secao_subrot();
void analisar_definicao_subrotina();
void analisar_lista_parametros();
void analisar_bloco_comandos_principal();
void analisar_bloco_comandos();
void analisar_expressao_aritmetica();
void analisar_expressao_relacional();
void analisar_expressao_logica();
void analisar_fator();
void analisar_termo();
void analisar_comando_read();
void analisar_comando_write();
void analisar_comando_if();
void analisar_comando_for();
void analisar_comando_while();
void analisar_comando_repeat_until();
void analisar_atribuicao();
void analisar_comando();
void analisar_comando_return();
void analisar_chamada_funcao();

// Variável global para armazenar o token atual
TInfoAtomo token_atual;

// --- Funções Auxiliares ---
void proximo_token() {
    token_atual = obter_atomo();
    // Para depuração:
    // printf("Token: %s, Tipo: %d, Linha: %d\n", token_atual.lexema, token_atual.tipo, token_atual.linha);
}

void erro_sintatico(const char* mensagem) {
    printf("Erro sintático na linha %d: %s (token inesperado: '%s', tipo: %d)\n", token_atual.linha, mensagem, token_atual.lexema, token_atual.tipo);
    exit(1);
}

void casar_token(TAtomo tipo_esperado, const char* lexema_esperado) {
    if (token_atual.tipo == tipo_esperado) {
        if (lexema_esperado == NULL || strcmp(token_atual.lexema, lexema_esperado) == 0) {
            proximo_token();
        } else {
            erro_sintatico("Lexema inesperado.");
        }
    } else {
        erro_sintatico("Tipo de token inesperado.");
    }
}

// --- Funções de Análise de Expressões (Implementação Completa e Corrigida) ---
void analisar_chamada_funcao() {
    casar_token(T_DELIM, "(");
    if (token_atual.tipo != T_DELIM || strcmp(token_atual.lexema, ")") != 0) {
        analisar_expressao_logica();
        while (token_atual.tipo == T_DELIM && strcmp(token_atual.lexema, ",") == 0) {
            proximo_token();
            analisar_expressao_logica();
        }
    }
    casar_token(T_DELIM, ")");
}

void analisar_fator() {
    if (token_atual.tipo == T_LITERAL_INT || token_atual.tipo == T_LITERAL_FLOAT || token_atual.tipo == T_LITERAL_CHAR || token_atual.tipo == T_LITERAL_STRING) {
        proximo_token();
    } else if (token_atual.tipo == T_ID) {
        proximo_token();
        if (token_atual.tipo == T_DELIM && strcmp(token_atual.lexema, "(") == 0) {
            analisar_chamada_funcao();
        }
    } else if (token_atual.tipo == T_DELIM && strcmp(token_atual.lexema, "(") == 0) {
        proximo_token();
        analisar_expressao_logica();
        casar_token(T_DELIM, ")");
    } else {
        erro_sintatico("Fator inválido em expressão.");
    }
}

void analisar_termo() {
    analisar_fator();
    while (token_atual.tipo == T_OP_ARIT && (strcmp(token_atual.lexema, "*") == 0 || strcmp(token_atual.lexema, "/") == 0)) {
        proximo_token();
        analisar_fator();
    }
}

void analisar_expressao_aritmetica() {
    analisar_termo();
    while (token_atual.tipo == T_OP_ARIT && (strcmp(token_atual.lexema, "+") == 0 || strcmp(token_atual.lexema, "-") == 0)) {
        proximo_token();
        analisar_termo();
    }
}

void analisar_expressao_relacional() {
    analisar_expressao_aritmetica();
    if (token_atual.tipo == T_OP_REL) {
        proximo_token();
        analisar_expressao_aritmetica();
    }
}

void analisar_expressao_logica() {
    analisar_expressao_relacional();
    while (token_atual.tipo == T_OP_LOG) {
        proximo_token();
        analisar_expressao_logica();
    }
}

void analisar_expressao() {
    analisar_expressao_logica();
}

// --- Funções de Análise de Comandos (Com Correções) ---
void analisar_comando_read() {
    casar_token(T_READ, NULL);
    casar_token(T_DELIM, "(");
    casar_token(T_ID, NULL);
    casar_token(T_DELIM, ")");
    casar_token(T_DELIM, ";");
}

void analisar_comando_write() {
    casar_token(T_WRITE, NULL);
    casar_token(T_DELIM, "(");
    analisar_expressao();
    while (token_atual.tipo == T_DELIM && strcmp(token_atual.lexema, ",") == 0) {
        proximo_token();
        analisar_expressao();
    }
    casar_token(T_DELIM, ")");
    casar_token(T_DELIM, ";");
}

void analisar_comando_if() {
    casar_token(T_IF, NULL);
    casar_token(T_DELIM, "(");
    analisar_expressao();
    casar_token(T_DELIM, ")");
    casar_token(T_THEN, NULL);
    analisar_bloco_comandos();
    if (token_atual.tipo == T_ELSE) {
        proximo_token();
        analisar_bloco_comandos();
    }
    casar_token(T_DELIM, ";");
}

void analisar_comando_for() {
    casar_token(T_FOR, NULL);
    casar_token(T_DELIM, "(");
    if (token_atual.tipo == T_ID) {
        analisar_atribuicao();
    }
    casar_token(T_DELIM, ";");
    analisar_expressao();
    casar_token(T_DELIM, ";");
    analisar_comando();
    casar_token(T_DELIM, ")");
    analisar_bloco_comandos();
    casar_token(T_DELIM, ";");
}

void analisar_comando_while() {
    casar_token(T_WHILE, NULL);
    casar_token(T_DELIM, "(");
    analisar_expressao();
    casar_token(T_DELIM, ")");
    analisar_bloco_comandos();
    casar_token(T_DELIM, ";");
}

void analisar_comando_repeat_until() {
    casar_token(T_REPEAT, NULL);
    analisar_bloco_comandos();
    casar_token(T_UNTIL, NULL);
    casar_token(T_DELIM, "(");
    analisar_expressao();
    casar_token(T_DELIM, ")");
    casar_token(T_DELIM, ";");
}

void analisar_atribuicao() {
    casar_token(T_ID, NULL);
    casar_token(T_OP_ATRIB, NULL);
    analisar_expressao();
    casar_token(T_DELIM, ";");
}

void analisar_comando_return() {
    casar_token(T_RETURN, NULL);
    analisar_expressao();
    casar_token(T_DELIM, ";");
}

void analisar_comando() {
    if (token_atual.tipo == T_READ) {
        analisar_comando_read();
    } else if (token_atual.tipo == T_WRITE) {
        analisar_comando_write();
    } else if (token_atual.tipo == T_IF) {
        analisar_comando_if();
    } else if (token_atual.tipo == T_FOR) {
        analisar_comando_for();
    } else if (token_atual.tipo == T_WHILE) {
        analisar_comando_while();
    } else if (token_atual.tipo == T_REPEAT) {
        analisar_comando_repeat_until();
    } else if (token_atual.tipo == T_ID) {
        analisar_atribuicao();
    } else if (token_atual.tipo == T_BEGIN) {
        analisar_bloco_comandos_principal();
    } else if (token_atual.tipo == T_RETURN) {
        analisar_comando_return();
    } else {
        erro_sintatico("Comando não reconhecido.");
    }
}

// --- Funções de Análise de Estrutura do Programa ---
void analisar_programa() {
    casar_token(T_PRG, NULL);
    casar_token(T_ID, NULL);
    casar_token(T_DELIM, ";");

    while (token_atual.tipo == T_VAR || token_atual.tipo == T_SUBROT) {
        if (token_atual.tipo == T_VAR) {
            analisar_secao_var();
        } else if (token_atual.tipo == T_SUBROT) {
            analisar_secao_subrot();
        }
    }

    analisar_bloco_comandos_principal();
    casar_token(T_DELIM, ".");

    printf("Análise sintática concluída com sucesso.\n");
}

void analisar_secao_var() {
    casar_token(T_VAR, NULL);
    while (token_atual.tipo == T_INT || token_atual.tipo == T_FLOAT || token_atual.tipo == T_CHAR) {
        analisar_declaracao_variavel();
    }
}

void analisar_declaracao_variavel() {
    proximo_token();
    casar_token(T_ID, NULL);
    while (token_atual.tipo == T_DELIM && strcmp(token_atual.lexema, ",") == 0) {
        proximo_token();
        casar_token(T_ID, NULL);
    }
    casar_token(T_DELIM, ";");
}

void analisar_secao_subrot() {
    casar_token(T_SUBROT, NULL);
    analisar_definicao_subrotina();
}

void analisar_definicao_subrotina() {
    proximo_token();
    casar_token(T_ID, NULL);
    casar_token(T_DELIM, "(");
    if (token_atual.tipo != T_DELIM || strcmp(token_atual.lexema, ")") != 0) {
        analisar_lista_parametros();
    }
    casar_token(T_DELIM, ")");
    if (token_atual.tipo == T_VAR) {
        analisar_secao_var();
    }
    if (token_atual.tipo == T_SUBROT) {
        analisar_secao_subrot();
    }
    analisar_bloco_comandos();
    casar_token(T_DELIM, ";");
}

void analisar_lista_parametros() {
    proximo_token();
    casar_token(T_ID, NULL);
    while (token_atual.tipo == T_DELIM && strcmp(token_atual.lexema, ",") == 0) {
        proximo_token();
        proximo_token();
        casar_token(T_ID, NULL);
    }
}

void analisar_bloco_comandos_principal() {
    casar_token(T_BEGIN, NULL);
    while (token_atual.tipo != T_END) {
        analisar_comando();
    }
    casar_token(T_END, NULL);
}

void analisar_bloco_comandos() {
    if (token_atual.tipo == T_BEGIN) {
        analisar_bloco_comandos_principal();
    } else {
        analisar_comando();
    }
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
    proximo_token();
    analisar_programa();
    fclose(arquivo);
    return 0;
}