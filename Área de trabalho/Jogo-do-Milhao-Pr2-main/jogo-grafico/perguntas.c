#include "quiz.h"

void F_Limpa_Buffer(void) 
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}
//void F_Limpa_Buffer() 


int F_Ler_Inteiro(void)
{
    char linha[64];
    char *fim;

    while (1)
    {
        if (fgets(linha, sizeof(linha), stdin) == NULL)
        {
            printf("\n[INFO] Entrada encerrada. Saindo.\n");
            exit(0);
        }
        //linha longa demais: descarta o resto
        if (strchr(linha, '\n') == NULL) F_Limpa_Buffer();

        long v = strtol(linha, &fim, 10);
        if (fim != linha && (*fim == '\n' || *fim == '\0' || *fim == ' ')) return (int) v;

        printf("Entrada inválida! Digite um número: ");
    }
}
//int F_Ler_Inteiro(void)


char F_Ler_Char(void)
{
    char linha[64];

    while (1)
    {
        if (fgets(linha, sizeof(linha), stdin) == NULL)
        {
            printf("\n[INFO] Entrada encerrada. Saindo.\n");
            exit(0);
        }
        if (strchr(linha, '\n') == NULL) F_Limpa_Buffer();

        for (int i = 0; linha[i] != '\0'; i++)
        {
            if (!isspace((unsigned char) linha[i])) return (char) toupper((unsigned char) linha[i]);
        }
        printf("Entrada vazia! Tente novamente: ");
    }
}
//char F_Ler_Char(void)


void F_Ler_String(char *destino, int tamanho) 
{
    while (1)
    {
        if (fgets(destino, tamanho, stdin) == NULL)
        {
            printf("\n[INFO] Entrada encerrada. Saindo.\n");
            exit(0);
        }
        //linha maior que o buffer: descarta o excedente
        if (strchr(destino, '\n') == NULL) F_Limpa_Buffer();

        destino[strcspn(destino, "\r\n")] = '\0';
        if (destino[0] != '\0') break;

        printf("O campo não pode ficar vazio. Digite novamente: ");
    }

    for (int i = 0; destino[i] != '\0'; i++) 
    {
        if (destino[i] == ',') destino[i] = ';';
    }
    //for (int i = 0; destino[i] != '\0'; i++) 
}
//void F_Ler_String(char *destino, int tamanho) 


const char* F_Nome_Dificuldade(int nivel) 
{
    switch (nivel) 
    {
        case 1: return "1 - Muito Fácil";
        case 2: return "2 - Fácil";
        case 3: return "3 - Médio";
        case 4: return "4 - Difícil";
        case 5: return "5 - Muito Difícil";
        default: return "Desconhecido";
    }
    //switch (nivel) 
}
//const char* F_Nome_Dificuldade(int nivel) 


void F_Exibir_Perguntas(Quiz q, int indice) 
{
    printf("\n\n");
    if (indice >= 0) printf("ID: [%d] | ", indice + 1);

    printf("Nível: %s\n", F_Nome_Dificuldade(q.dificuldade));
    printf("Enunciado: %s\n", q.enunciado);
    printf("  A) %s\n", q.alternativas[0]);
    printf("  B) %s\n", q.alternativas[1]);
    printf("  C) %s\n", q.alternativas[2]);
    printf("  D) %s\n", q.alternativas[3]);
    printf("Resposta Correta: %c\n\n", q.alternativa_correta);
}
//void F_Exibir_Perguntas(Quiz q, int indice) 


Quiz* F_Inserir_Pergunta(Quiz *questionario, int *total_perguntas) 
{
    Quiz *temp = (Quiz*) realloc(questionario, (*total_perguntas + 1) * sizeof(Quiz));

    if (temp == NULL) 
    {
        printf("Erro: Falha ao alocar memória!\n");
        return questionario;
    }
    //if (temp == NULL) 
    questionario = temp;

    Quiz nova;
    printf("\nCADASTRAR NOVA PERGUNTA (Vírgulas serão convertidas em ';')\n");
    printf("Enunciado: ");
    F_Ler_String (nova.enunciado, max_enunciado);

    for (int i = 0; i < 4; i++) 
    {
        printf("Alternativa %c: ", 'A' + i);
        F_Ler_String (nova.alternativas[i], max_atlernativa);
    }
    //for (int i = 0; i < 4; i++) 

    do //while (nova.alternativa_correta < 'A' || nova.alternativa_correta > 'D');
    {
        printf("Letra da alternativa correta (A, B, C ou D): ");
        nova.alternativa_correta = F_Ler_Char();
    } 
    while (nova.alternativa_correta < 'A' || nova.alternativa_correta > 'D');

    do //while (nova.dificuldade < 1 || nova.dificuldade > 5);
    {
        printf("Nível de dificuldade (1-Muito Fácil, 2-Fácil, 3-Médio, 4-Difícil, 5-Muito Difícil): ");
        nova.dificuldade = F_Ler_Inteiro();
    } while (nova.dificuldade < 1 || nova.dificuldade > 5);

    questionario[*total_perguntas] = nova;
    (*total_perguntas)++;

    printf("Pergunta cadastrada com sucesso!\n");


    return questionario;
}
//Quiz* F_Inserir_Pergunta(Quiz *questionario, int *total_perguntas) 


void F_Listar_Perguntas(Quiz *questionario, int total_perguntas) 
{
    if (total_perguntas == 0) 
    {
        printf("\nNenhuma pergunta cadastrada.\n");
        return;
    }
    //if (total_perguntas == 0) 

    printf("\nLISTA DE TODAS AS PERGUNTAS (%d)\n", total_perguntas);
    for (int i = 0; i < total_perguntas; i++) 
    {
        F_Exibir_Perguntas(questionario[i], i);
    }
    //for (int i = 0; i < total_perguntas; i++)
}
//void listarPerguntas(Quiz *questionario, int total_perguntas) 


void F_Pesquisar_Por_Texto(Quiz *questionario, int total_perguntas) 
{
    if (total_perguntas == 0) 
    {
        printf("\nNenhuma pergunta cadastrada.\n");
        return;
    }
    //if (total_perguntas == 0) 

    char termo[100];
    
    printf("\nDigite uma palavra ou trecho do enunciado para buscar: ");
    F_Ler_String (termo, 100);

    int encontradas = 0;
    for (int i = 0; i < total_perguntas; i++) 
    {
        if (strstr(questionario[i].enunciado, termo) != NULL) 
        {
            F_Exibir_Perguntas(questionario[i], i);
            encontradas++;
        }
        //if (strstr(questionario[i].enunciado, termo) != NULL) 
    }
    //for (int i = 0; i < total_perguntas; i++) 

    if (encontradas == 0) 
    {
        printf("Nenhuma pergunta encontrada com o termo '%s'.\n", termo);
    }
    //if (encontradas == 0) 
}
//void pesquisarPorTexto(Quiz *questionario, int total_perguntas) 


void F_Navegar_Por_Dificuldade(Quiz *questionario, int total_perguntas) 
{
    if (total_perguntas == 0) 
    {
        printf("\nNenhuma pergunta cadastrada.\n");
        return;
    }
    //if (total_perguntas == 0) 


    int nivel;
    printf("\nEscolha o nível para filtrar (1-Muito Fácil, 2-Fácil, 3-Médio, 4-Difícil, 5-Muito Difícil): ");
    nivel = F_Ler_Inteiro();

    if (nivel < 1 || nivel > 5) 
    {
        printf("Nível inválido!\n");
        return;
    }
    //if (nivel < 1 || nivel > 5) 

    int *indices_filtrados = NULL;
    int qtd_filtrada = 0;

    for (int i = 0; i < total_perguntas; i++) 
    {
        if (questionario[i].dificuldade == nivel) 
        {
            int *tmp = (int*) realloc(indices_filtrados, (qtd_filtrada + 1) * sizeof(int));
            if (tmp == NULL)
            {
                printf("Erro: Falha ao alocar memória!\n");
                free(indices_filtrados);
                return;
            }
            indices_filtrados = tmp;
            indices_filtrados[qtd_filtrada] = i;
            qtd_filtrada++;
        }
        //if (questionario[i].dificuldade == nivel) 
    }
    //for (int i = 0; i < total_perguntas; i++) 

    if (qtd_filtrada == 0) 
    {
        printf("Não há perguntas cadastradas no nível %s.\n", F_Nome_Dificuldade(nivel));
        return;
    }
    //if (qtd_filtrada == 0)

    int atual = 0;
    char cmd;
    do //while (cmd != 'S');
    {
        printf("\nNAVEGANDO: %s (Pergunta %d de %d)", F_Nome_Dificuldade(nivel), atual + 1, qtd_filtrada);
        F_Exibir_Perguntas(questionario[indices_filtrados[atual]], indices_filtrados[atual]);
        printf("[A] Anterior | [P] Próxima | [S] Sair da navegação: ");
        cmd = F_Ler_Char();

        if (cmd == 'P') 
        {
            if (atual < qtd_filtrada - 1) atual++;
            else printf("\n>> Você já está na última pergunta deste nível!\n");
        } 
        // if (cmd == 'P')
        else if (cmd == 'A') 
        {
            if (atual > 0) atual--;
            else printf("\n>> Você já está na primeira pergunta deste nível!\n");
        } 
        //else if (cmd == 'A')
    } while (cmd != 'S');


    free(indices_filtrados);
}
//void navegarPorDificuldade(Quiz *questionario, int total_perguntas)


void F_Alterar_Pergunta(Quiz *questionario, int total_perguntas) 
{
    if (total_perguntas == 0) 
    {
        printf("\nNenhuma pergunta cadastrada.\n");
        return;
    }
    //void alterarPergunta(Quiz *questionario, int total_perguntas) 
    
    int id;
    printf("\nDigite o ID da pergunta que deseja alterar (1 a %d): ", total_perguntas);
    id = F_Ler_Inteiro();

    if (id < 1 || id > total_perguntas) 
    {
        printf("ID inválido!\n");
        return;
    }
    //if (id < 1 || id > total_perguntas) 

    int idx = id - 1;
    printf("Pergunta atual:");
    F_Exibir_Perguntas(questionario[idx], idx);

    printf("\nINSIRA OS NOVOS DADOS\n");
    printf("Novo Enunciado: ");
    F_Ler_String(questionario[idx].enunciado, max_enunciado);

    for (int i = 0; i < 4; i++) 
    {
        printf("Nova Alternativa %c: ", 'A' + i);
        F_Ler_String(questionario[idx].alternativas[i], max_atlernativa);
    }
    //for (int i = 0; i < 4; i++) 

    do //while (questionario[idx].alternativa_correta < 'A' || questionario[idx].alternativa_correta > 'D');
    {
        printf("Nova letra correta (A, B, C ou D): ");
        questionario[idx].alternativa_correta = F_Ler_Char();
    } 
    while (questionario[idx].alternativa_correta < 'A' || questionario[idx].alternativa_correta > 'D');


    do //while (questionario[idx].dificuldade < 1 || questionario[idx].dificuldade > 5);
    {
        printf("Novo nível de dificuldade (1 a 5): ");
        questionario[idx].dificuldade = F_Ler_Inteiro();
    } 
    while (questionario[idx].dificuldade < 1 || questionario[idx].dificuldade > 5);

    printf("Pergunta alterada com sucesso!\n");
}
//void alterarPergunta(Quiz *questionario, int total_perguntas) 


Quiz* F_Excluir_Pergunta(Quiz *questionario, int *total_perguntas) 
{
    if (*total_perguntas == 0) 
    {
        printf("\nNenhuma pergunta cadastrada.\n");
        return questionario;
    }
    //if (*total_perguntas == 0) 

    int id;
    printf("\nDigite o ID da pergunta que deseja excluir (1 a %d): ", *total_perguntas);
    id = F_Ler_Inteiro();

    if (id < 1 || id > *total_perguntas) 
    {
        printf("ID inválido!\n");
        return questionario;
    }
    //if (id < 1 || id > *total_perguntas) 

    int idx = id - 1;
    for (int i = idx; i < (*total_perguntas) - 1; i++) 
    {
        questionario[i] = questionario[i + 1];
    }
    //for (int i = idx; i < (*total_perguntas) - 1; i++) 


    (*total_perguntas)--;


    if (*total_perguntas == 0) 
    {
        free(questionario);
        questionario = NULL;
    } else {
        Quiz *temp = (Quiz*) realloc(questionario, (*total_perguntas) * sizeof(Quiz));
        if (temp != NULL) 
        {
            questionario = temp;
        }
        //if (temp != NULL) 
    }
    //if (*total_perguntas == 0)

    printf("Pergunta excluída com sucesso!\n");
    return questionario;
}
//Quiz* excluirPergunta(Quiz *questionario, int *total_perguntas) 