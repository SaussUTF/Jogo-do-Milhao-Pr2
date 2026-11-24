//>Security Guard
#ifndef QUIZ_H
#define QUIZ_H


//>Includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif


//>Define
#define max_enunciado 200
#define max_atlernativa 120
#define arq_bin "perguntas.bin"
#define arq_csv_inicial "perguntas_iniciais.csv"
#define arq_csv_exportada "perguntas_exportadas.csv"



//>Struct das perguntas
typedef struct
{
    int dificuldade; 
    char enunciado[max_enunciado];
    char alternativas[4][max_atlernativa];
    char alternativa_correta; 
} 
Quiz;



//>Funões de arquivo/persistência (arquivo.c)
Quiz* F_Inicializa_Sistema(FILE **arq_perguntas, int *total_perguntas);
Quiz* F_Carrega_Binario(FILE **arq_perguntas, int *total_perguntas); //Carrega do binário
void F_Salvar_Binario(FILE **arq_perguntas, Quiz *questionario, int total_perguntas); //Salva no binário
Quiz* F_Importar_CSV(const char *nome_arquivo, int *total_perguntas); //Importa do CSV
void F_Exportar_CSV(Quiz *questionario, int total_perguntas, const char *nome_arquivo); //Expporta para CSV
void F_Gera_CSV_Exemplo_80_Perguntas(const char *nome_arquivo);



//>Funções de perguntas auxiliares (perguntas.c)
void F_Limpa_Buffer(void);
int F_Ler_Inteiro(void);   //Lê um inteiro com segurança (linha inteira)
char F_Ler_Char(void);     //Lê o primeiro caractere (maiúsculo) de uma linha
void F_Ler_String(char *destino, int tamanho); 
const char* F_Nome_Dificuldade(int nivel);
void F_Exibir_Perguntas(Quiz q, int indice);
Quiz* F_Inserir_Pergunta(Quiz *questionario, int *total_perguntas);
void F_Listar_Perguntas(Quiz *questionario, int total_perguntas);
void F_Pesquisar_Por_Texto(Quiz *questionario, int total_perguntas);
void F_Navegar_Por_Dificuldade(Quiz *questionario, int total_perguntas);
void F_Alterar_Pergunta(Quiz *questionario, int total_perguntas);
Quiz* F_Excluir_Pergunta(Quiz *questionario, int *total_perguntas);



//>Funções do jogo (jogo.c)
void F_Jogar_Quiz(Quiz *questionario, int total_perguntas);

#endif