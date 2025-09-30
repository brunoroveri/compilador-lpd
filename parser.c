#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scanner.c" // Inclui seu analisador léxico

// Variável global para armazenar o token atual
TInfoAtomo token_atual;

// --- Funções Auxiliares ---

// Função para avançar para o próximo token
void proximo_token() {
    token_atual = obter_atomo();
    // Você pode descomentar a linha abaixo para depuração:
    // printf("Token atual: %s, Tipo: %d, Linha: %d\n", token_atual.lexema, token_atual.tipo, token_atual.linha);
}

// Função para reportar um erro sintático
void erro_sintatico(const char* mensagem) {
    printf("Erro sintático na linha %d: %s (token inesperado: '%s', tipo: %d)\n", token_atual.linha, mensagem, token_atual.lexema, token_atual.tipo);
    exit(1);
}

// Função para verificar se o token atual é o esperado e avançar
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

// --- Funções de Análise de Comandos ---

void analisar_expressao() {
    // Esta função é um dos pontos mais complexos do parser e
    // precisa ser implementada com base na precedência de operadores.
    // Por enquanto, vamos simplificar para aceitar IDs, literais e parênteses.
    if (token_atual.tipo == T_ID || token_atual.tipo == T_LITERAL_INT || token_atual.tipo == T_LITERAL_FLOAT || token_atual.tipo == T_LITERAL_CHAR) {
        proximo_token();
        // Lógica para analisar operadores e outros termos da expressão
        // ... (você precisará expandir isso)
    } else if (token_atual.tipo == T_DELIM && strcmp(token_atual.lexema, "(") == 0) {
        proximo_token();
        analisar_expressao();
        casar_token(T_DELIM, ")");
    } else {
        erro_sintatico("Expressão inválida.");
    }
}

void analisar_comando_read() {
    casar_token(T_READ, NULL);
    casar_token(T_DELIM, "(");
    casar_token(T_ID, NULL); // O comando read só suporta variáveis
    casar_token(T_DELIM, ")");
    casar_token(T_DELIM, ";");
}

void analisar_comando_write() {
    casar_token(T_WRITE, NULL);
    casar_token(T_DELIM, "(");
    analisar_expressao(); // Aceita ID, literal, ou outra expressão
    // Loop para suportar múltiplos parâmetros
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
    analisar_expressao(); // A expressão deve ser lógica ou numérica
    casar_token(T_DELIM, ")");
    casar_token(T_THEN, NULL);
    analisar_bloco_comandos();
    // Cláusula else é opcional
    if (token_atual.tipo == T_ELSE) {
        proximo_token();
        analisar_bloco_comandos();
    }
    casar_token(T_DELIM, ";");
}

void analisar_atribuicao() {
    // A gramática da atribuição é: <identificador> <- <expressão>;
    casar_token(T_ID, NULL);
    casar_token(T_OP_ATRIB, NULL);
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
        // ... Você precisa implementar a análise do comando FOR
        proximo_token();
        // ...
    } else if (token_atual.tipo == T_WHILE) {
        // ... Você precisa implementar a análise do comando WHILE
        proximo_token();
        // ...
    } else if (token_atual.tipo == T_REPEAT) {
        // ... Você precisa implementar a análise do comando REPEAT
        proximo_token();
        // ...
    } else if (token_atual.tipo == T_ID) {
        // Se o token atual é um identificador, pode ser uma atribuição
        // ou uma chamada de função que retorna um valor.
        TInfoAtomo proximo_token_temp = obter_atomo(); // Peek no próximo token
        ungetc(proximo_token_temp.lexema[0], arquivo); // Devolve o token
        if (proximo_token_temp.tipo == T_OP_ATRIB) {
            analisar_atribuicao();
        } else {
            erro_sintatico("Comando inválido. Atribuição ou chamada de função esperada.");
        }
    } else if (token_atual.tipo == T_BEGIN) {
        // É um bloco de comandos
        analisar_bloco_comandos();
    } else {
        erro_sintatico("Comando não reconhecido.");
    }
}

// --- Funções de Análise de Estrutura do Programa (Do esqueleto anterior) ---

void analisar_programa();
void analisar_secao_var();
void analisar_declaracao_variavel();
void analisar_secao_subrot();
void analisar_definicao_subrotina();
void analisar_lista_parametros();
void analisar_bloco_comandos_principal();
void analisar_bloco_comandos();

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
    TAtomo tipo_var = token_atual.tipo;
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
    while (token_atual.tipo == T_INT || token_atual.tipo == T_FLOAT || token_atual.tipo == T_CHAR || token_atual.tipo == T_VOID) {
        analisar_definicao_subrotina();
    }
}

void analisar_definicao_subrotina() {
    proximo_token(); // Consome o tipo de retorno
    casar_token(T_ID, NULL);
    casar_token(T_DELIM, "(");
    if (token_atual.tipo != T_DELIM || strcmp(token_atual.lexema, ")") != 0) {
        analisar_lista_parametros();
    }
    casar_token(T_DELIM, ")");
    analisar_bloco_comandos();
    casar_token(T_DELIM, ";");
}

void analisar_lista_parametros() {
    proximo_token(); // Consome o tipo do parâmetro
    casar_token(T_ID, NULL);
    while (token_atual.tipo == T_DELIM && strcmp(token_atual.lexema, ",") == 0) {
        proximo_token();
        proximo_token(); // Consome o tipo
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