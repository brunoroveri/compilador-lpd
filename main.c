#include <stdio.h>
#include <stdlib.h>
#include "scanner.h"

// protótipos do parser
void iniciar_parser(FILE *fp);
void analisar_programa_public(void);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <arquivo.lpd>\n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        perror("Erro ao abrir arquivo");
        return 1;
    }

    iniciar_parser(fp);
    analisar_programa_public();
    fclose(fp);

    printf("OK: análise sintática concluída.\n");
    return 0;
}