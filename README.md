## Compilador LPD – AnaLex + Parser

Implementação de um analisador léxico (scanner) e analisador sintático (parser) em C para a linguagem LPD.
Bruno Roveri - 10401752
Ian Merlini -


# Como compilar

No terminal (Linux, WSL ou Codespaces), rodar:

gcc -std=c11 -Wall -Wextra -O2 parser.c scanner.c main.c -o meu_compilador

Vai gerar o executável meu_compilador.


# Como executar

Para rodar um arquivo .lpd, use:

./meu_compilador exemplo_teste1.lpd

ou qualquer outro arquivo da linguagem.


# Estrutura
parser.c    -> analisador sintático
scanner.c   -> analisador léxico
scanner.h   -> definição de tokens e TInfoAtomo
main.c      -> função main, abre o arquivo e chama o parser
exemplo_teste*.lpd -> casos de teste
