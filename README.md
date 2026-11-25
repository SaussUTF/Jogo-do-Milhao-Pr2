# Jogo do Milhão (interface gráfica)
GRUPO:



       ENZO ANDRE

       NATHAN SAUSS

       LUCAS REZENDE


Projeto em C com interface gráfica usando Raylib. O código principal da interface está em `jogo-grafico/show.c`.

## Estrutura
- `jogo-grafico/`: código C, cabeçalho, CSV inicial e recursos gráficos/áudio.
- `jogo-grafico/perguntas_iniciais.csv`: banco inicial com 80 perguntas (20 nível 1, 20 nível 2, 15 nível 3, 15 nível 4 e 10 nível 5).

## Compilar e executar (Linux)
É necessário ter GCC e Raylib instalados. Na raiz do repositório:

```bash
make
make run
```

Ou dentro da pasta gráfica:

```bash
cd jogo-grafico
make
make run
```

## Limpeza
```bash
make clean
```

A primeira execução importa as perguntas do CSV e cria `perguntas.bin`. Nas execuções seguintes, os dados são lidos do binário, conforme o requisito do trabalho. Para reimportar o CSV inicial, remova `jogo-grafico/perguntas.bin` e execute novamente.

## CSV
Formato com campos separados por vírgula:
`Dificuldade,Enunciado,AlternativaA,AlternativaB,AlternativaC,AlternativaD,Correta`

O CSV contém perguntas iniciais de temática Cultura Geek/Pop e tecnologia.
