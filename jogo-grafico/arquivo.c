#include "quiz.h"

Quiz* F_Inicializa_Sistema(FILE **arq_perguntas, int *total_perguntas) 
{
    *arq_perguntas = fopen(arq_bin, "rb");

    
    //Se não for a primeira execução
    if (*arq_perguntas != NULL) 
    {
        fclose(*arq_perguntas);
        *arq_perguntas = NULL;
        printf("[INFO] Arquivo binário '%s' encontrado.\n", arq_bin);

        Quiz *carregado = F_Carrega_Binario(arq_perguntas, total_perguntas);
        if (carregado != NULL && *total_perguntas > 0) return carregado;

        //binário vazio/corrompido: cai para a importação do CSV
        free(carregado);
        printf("[AVISO] Binário vazio ou inválido. Importando do CSV.\n");
    }
    //if (*arq_perguntas != NULL) 


    //Se for a primeira execução
    printf("[INFO] Primeira execução (sem arquivo binário).\n");
    FILE *f_csv = fopen(arq_csv_inicial, "r");
    if (f_csv == NULL) 
    {
        printf("[AVISO] '%s' não encontrado. Gerando arquivo CSV padrão com 80 perguntas.\n", arq_csv_inicial);
        F_Gera_CSV_Exemplo_80_Perguntas(arq_csv_inicial);
    } else {
        fclose(f_csv);
    }
    //if (f_csv == NULL) 


    printf("[INFO] Importando perguntas iniciais de '%s'.\n", arq_csv_inicial);
    return F_Importar_CSV(arq_csv_inicial, total_perguntas);
}
//Quiz* F_Inicializa_Sistema(FILE **arq_perguntas, int *total_perguntas) 


Quiz* F_Carrega_Binario(FILE **arq_perguntas, int *total_perguntas) 
{
    *arq_perguntas = fopen(arq_bin, "rb");
    if (!(*arq_perguntas)) 
    {
        *total_perguntas = 0;
        return NULL;
    }
    //if (!(*arq_perguntas))


    if (fread(total_perguntas, sizeof(int), 1, *arq_perguntas) != 1 || *total_perguntas <= 0) 
    {
        fclose(*arq_perguntas);
        *arq_perguntas = NULL;
        *total_perguntas = 0;
        return NULL;
    }
    //if (*total_perguntas <= 0)


    Quiz *questionario = (Quiz*) malloc((*total_perguntas) * sizeof(Quiz));
    if (questionario != NULL) 
    {
        size_t lidas = fread(questionario, sizeof(Quiz), *total_perguntas, *arq_perguntas);
        if (lidas != (size_t) *total_perguntas) 
        {
            printf("[AVISO] Arquivo binário incompleto: %zu de %d perguntas lidas.\n", lidas, *total_perguntas);
            *total_perguntas = (int) lidas;
        }
    } else {
        *total_perguntas = 0;
    }
    //if (questionario != NULL) 
    fclose(*arq_perguntas);
    

    *arq_perguntas = NULL;
    return questionario;
}
//Quiz* F_Carrega_Binario(FILE **arq_perguntas, int *total_perguntas) 


void F_Salvar_Binario(FILE **arq_perguntas, Quiz *questionario, int total_perguntas) 
{
    *arq_perguntas = fopen(arq_bin, "wb");
    if (!(*arq_perguntas)) 
    {
        printf("[ERRO] Não foi possível salvar o arquivo binário '%s'!\n", arq_bin);
        return;
    }
    //if (!(*arq_perguntas))

    fwrite(&total_perguntas, sizeof(int), 1, *arq_perguntas);
    if (total_perguntas > 0 && questionario != NULL) 
    {
        fwrite(questionario, sizeof(Quiz), total_perguntas, *arq_perguntas);
    }
    //if (total_perguntas > 0 && questionario != NULL)
    fclose(*arq_perguntas);
    *arq_perguntas = NULL;
}
//void F_Salvar_Binario(FILE **arq_perguntas, Quiz *questionario, int total_perguntas) 


Quiz* F_Importar_CSV(const char *nome_arquivo, int *total_perguntas) 
{
    FILE *f = fopen(nome_arquivo, "r");
    if (!f) 
    {
        printf("[ERRO] Falha ao abrir '%s'.\n", nome_arquivo);
        *total_perguntas = 0;
        return NULL;
    }
    //if (!f) 

    Quiz *questionario = NULL;
    *total_perguntas = 0;
    char linha[1024];

    if (fgets(linha, sizeof(linha), f)) 
    {
        if (strncmp(linha, "Dificuldade", 11) != 0 && strncmp(linha, "Enunciado", 9) != 0) 
        {
            rewind(f);
        }
        //if (strncmp(linha, "Dificuldade", 11) != 0 && strncmp(linha, "Enunciado", 9) != 0) 
    }
    //if (fgets(linha, sizeof(linha), f)) 

    while (fgets(linha, sizeof(linha), f)) 
    {
        linha[strcspn(linha, "\r\n")] = '\0';
        if (strlen(linha) == 0) continue;


        Quiz q;
        char *token = strtok(linha, ",");
        if (!token) continue;
        q.dificuldade = atoi(token);
        if (q.dificuldade < 1 || q.dificuldade > 5) continue;


        token = strtok(NULL, ",");
        if (!token) continue;
        strncpy(q.enunciado, token, max_enunciado - 1);
        q.enunciado[max_enunciado - 1] = '\0';


        int valido = 1;
        for (int i = 0; i < 4; i++) 
        {
            token = strtok(NULL, ",");
            if (!token) { valido = 0; break; }
            strncpy(q.alternativas[i], token, max_atlernativa - 1);
            q.alternativas[i][max_atlernativa - 1] = '\0';
        }
        //for (int i = 0; i < 4; i++) 
        if (!valido) continue;


        token = strtok(NULL, ",");
        if (!token) continue;
        q.alternativa_correta = (char) toupper((unsigned char) token[0]);
        if (q.alternativa_correta < 'A' || q.alternativa_correta > 'D') continue;

        
        Quiz *temp = (Quiz*) realloc(questionario, (*total_perguntas + 1) * sizeof(Quiz));
        if (!temp) 
        {
            printf("[ERRO] Memória insuficiente ao importar CSV.\n");
            break;
        }
        //if (!temp) 
        questionario = temp;
        questionario[*total_perguntas] = q;
        (*total_perguntas)++;
    }
    //while (fgets(linha, sizeof(linha), f)) 

    fclose(f);
    printf("[SUCESSO] %d perguntas importadas de '%s'.\n", *total_perguntas, nome_arquivo);
    return questionario;
}
//Quiz* F_Importar_CSV(const char *nome_arquivo, int *total_perguntas) 


void F_Exportar_CSV(Quiz *questionario, int total_perguntas, const char *nome_arquivo) 
{
    FILE *f = fopen(nome_arquivo, "w");
    if (!f) 
    {
        printf("[ERRO] Não foi possível criar o arquivo '%s'.\n", nome_arquivo);
        return;
    }
    //if (!f) 

    fprintf(f, "Dificuldade,Enunciado,AlternativaA,AlternativaB,AlternativaC,AlternativaD,Correta\n");

    for (int i = 0; i < total_perguntas; i++) 
    {
        fprintf(f, "%d,%s,%s,%s,%s,%s,%c\n",
                questionario[i].dificuldade,
                questionario[i].enunciado,
                questionario[i].alternativas[0],
                questionario[i].alternativas[1],
                questionario[i].alternativas[2],
                questionario[i].alternativas[3],
                questionario[i].alternativa_correta);
    }
    //for (int i = 0; i < total_perguntas; i++) 

    fclose(f);
    printf("\n[SUCESSO] %d perguntas exportadas para '%s'!\n", total_perguntas, nome_arquivo);
}
//void F_Exportar_CSV(Quiz *questionario, int total_perguntas, const char *nome_arquivo) 


void F_Gera_CSV_Exemplo_80_Perguntas(const char *nome_arquivo) 
{
    FILE *f = fopen(nome_arquivo, "w");
    if (!f) return;

    fprintf(f, "Dificuldade,Enunciado,AlternativaA,AlternativaB,AlternativaC,AlternativaD,Correta\n");
    
    int quantidades[6] = {0, 20, 20, 15, 15, 10};
    for (int niv = 1; niv <= 5; niv++) 
    {
        for (int q = 1; q <= quantidades[niv]; q++) 
        {
            fprintf(f, "%d,Pergunta exemplo %d do nível %d (Qual a alternativa A?),Opção Correta A,Opção Incorreta B,Opção Incorreta C,Opção Incorreta D,A\n", niv, q, niv);
        }
        //for (int q = 1; q <= quantidades[niv]; q++) 
    }
    //for (int niv = 1; niv <= 5; niv++) 
    fclose(f);
}
//void F_Gera_CSV_Exemplo_80_Perguntas(const char *nome_arquivo) 
