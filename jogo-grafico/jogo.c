#include "quiz.h" //Include que vai puxar todas as funções (tirando o F_Menu)


void F_Jogar_Quiz(Quiz *questionario, int total_perguntas) 
{
    int niveis_por_questao[15] = {1, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5};
    int quantidade_necessaria[6] = {0, 2, 2, 4, 4, 3};

    for (int niv = 1; niv <= 5; niv++) 
    {
        int cont = 0;

        for (int i = 0; i < total_perguntas; i++) 
        {
            if (questionario[i].dificuldade == niv) cont++;
        }
        //for (int i = 0; i < total_perguntas; i++) 

        if (cont < quantidade_necessaria[niv]) 
        {
            printf("\n[ERRO] Perguntas insuficientes para iniciar o jogo!\n");
            printf("O nível '%s' precisa de pelo menos %d perguntas (atual: %d).\n", 
            F_Nome_Dificuldade(niv), quantidade_necessaria[niv], cont);
            return;
        }
        //if (cont < quantidade_necessaria[niv]) 
    }
    //for (int niv = 1; niv <= 5; niv++) 

    int *usadas = (int*) calloc(total_perguntas, sizeof(int));
    if (usadas == NULL)
    {
        printf("Erro: Falha ao alocar memória!\n");
        return;
    }
    int marco_seguranca = 0;

    printf("\n========================================\n");
    printf("          INICIANDO O DESAFIO!          \n");
    printf("========================================\n");

    for (int q = 0; q < 15; q++) 
    {
        int numero_questao = q + 1;
        int nivel_atual = niveis_por_questao[q];

        if (numero_questao == 15) 
        {
            printf("\n*** ATENÇÃO: QUESTÃO FINAL (QUESTÃO 15)! ***\n");
        }

        int sorteada = -1;
        while (sorteada == -1) 
        {
            int r = rand() % total_perguntas;

            if (questionario[r].dificuldade == nivel_atual && usadas[r] == 0) 
            {
                sorteada = r;
                usadas[r] = 1;
            }
            //if (questionario[r].dificuldade == nivel_atual && usadas[r] == 0) 
        }
        //while (sorteada == -1) 

        printf("QUESTÃO %d / 15 | Dificuldade: %s\n", numero_questao, F_Nome_Dificuldade(nivel_atual));
        printf("Marco de Segurança atual: Questão %d\n", marco_seguranca);
        printf("%s\n\n", questionario[sorteada].enunciado);
        printf("  A) %s\n", questionario[sorteada].alternativas[0]);
        printf("  B) %s\n", questionario[sorteada].alternativas[1]);
        printf("  C) %s\n", questionario[sorteada].alternativas[2]);
        printf("  D) %s\n", questionario[sorteada].alternativas[3]);

        char resposta;
        do //while ((resposta < 'A' || resposta > 'D') && resposta != 'P');
        {
            printf("\nSua resposta (A, B, C ou D) ou [P] para Parar: ");
            resposta = F_Ler_Char();
        } while ((resposta < 'A' || resposta > 'D') && resposta != 'P');

        if (resposta == 'P') 
        {
            printf("\nVocê decidiu parar na questão %d! Você garantiu %d questões concluídas.\n", numero_questao, q);
            free(usadas);
            return;
        }
        //if (resposta == 'P') 

        if (resposta == questionario[sorteada].alternativa_correta) 
        {
            printf("\nCERTA RESPOSTA! <<\n");

            //marcos de segurança só valem depois de ACERTAR a questão
            if (numero_questao == 5) 
            {
                marco_seguranca = 5;
                printf("\n*** VOCÊ ALCANÇOU O 1º MARCO DE SEGURANÇA (QUESTÃO 5)! ***\n");
            } else if (numero_questao == 10) {
                marco_seguranca = 10;
                printf("\n*** VOCÊ ALCANÇOU O 2º MARCO DE SEGURANÇA (QUESTÃO 10)! ***\n");
            }
        } else {
            printf("\nRESPOSTA ERRADA! A correta era: %c <<\n", questionario[sorteada].alternativa_correta);
            if (marco_seguranca) printf("Fim de jogo! Você voltou ao Marco de Segurança da questão %d.\n", marco_seguranca);
            else printf("Fim de jogo! Você não alcançou nenhum Marco de Segurança.\n");
            free(usadas);
            return;
        }
        //(resposta == questionario[sorteada].alternativa_correta) 
    }
    //for (int q = 0; q < 15; q++) 

    printf("\n\nConcluído!\n\n");


    //libera memória
    free(usadas);
}
//void F_Jogar_Quiz(Quiz *questionario, int total_perguntas) 
