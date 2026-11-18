#include "quiz.h" //Include que vai puxar todas as funções (tirando o F_Menu)

/*
Para fazer:
- Separar funções por Bibliotecas
- Add interface gráfica
- Make file
*/



//!!MENU TEMPORÁRIO!!
Quiz* F_Menu(Quiz *questionario, int *total_perguntas, FILE **arq_perguntas) {
    int op_navegar = -1; //opção para acessar jogar, persquisar ou configuração de perguntas
    int op_pesquisa = -1; //opção para para navegar dentro de perguntas
    int op_configs = -1; //opção para para navegar dentro das configuração de perguntas
    //Menu 1 - Navegar
    do 
    {
        printf("[SISTEMA]\n\n");
        printf("Total de perguntas no banco (%d)\n", *total_perguntas);

        //Opções para selecionar
        printf("1. Jogar Quiz\n");
        printf("2. Visualizar Perguntas\n");
        printf("3. Gerenciar Perguntas\n");
        printf("0. Salvar no arquivo binário e Sair\n");
        printf("Escolha uma opção: ");
        op_navegar = F_Ler_Inteiro();

        //Opções selecionadas
        switch (op_navegar) 
        {
            // 1. Jogar Quiz
            case 1:
                F_Jogar_Quiz(questionario, *total_perguntas);
                break;
                //case 1 do switch (op_navegar) 

            //2. Visualizar Perguntas
            case 2:
                do 
                {
                    printf("[VISUALIZAR]\n\n");

                    //Opções para selecionar
                    printf("1. Listar todas as perguntas\n");
                    printf("2. Pesquisar pergunta por texto\n");
                    printf("3. Navegar perguntas por nível de dificuldade\n");
                    printf("0. Voltar ao menu principal\n");
                    printf("Escolha uma opção: ");
                    op_pesquisa = F_Ler_Inteiro();

                    //Opções selecionadas
                    switch (op_pesquisa) 
                    {
                        case 1: 
                            F_Listar_Perguntas(questionario, *total_perguntas); 
                            break;

                        case 2: 
                            F_Pesquisar_Por_Texto(questionario, *total_perguntas); 
                            break;

                        case 3: 
                            F_Navegar_Por_Dificuldade(questionario, *total_perguntas); 
                            break;

                        case 0: 
                            break;

                        default: 
                            printf("Opção inválida!\n");
                    }
                    //switch (op_pesquisa)
                } 
                while (op_pesquisa != 0);
                break;
                //case 2 do switch (op_navegar) 

            //3. Gerenciar Perguntas
            case 3:
                do {
                    printf("[GERENCIAR]\n\n");

                    //Opções para selecionar
                    printf("1. Inserir nova pergunta\n");
                    printf("2. Alterar uma pergunta existente\n");
                    printf("3. Excluir uma pergunta\n");
                    printf("4. Exportar perguntas atuais para arquivo .CSV\n");
                    printf("0. Voltar ao menu principal\n");
                    printf("Escolha uma opção: ");
                    op_configs = F_Ler_Inteiro();

                    //Opções selecionadas
                    switch (op_configs) {
                        case 1: 
                            questionario = F_Inserir_Pergunta(questionario, total_perguntas); 
                            break;

                        case 2: 
                            F_Alterar_Pergunta(questionario, *total_perguntas); 
                            break;

                        case 3: 
                            questionario = F_Excluir_Pergunta(questionario, total_perguntas); 
                            break;
                        
                        case 4: 
                            F_Exportar_CSV(questionario, *total_perguntas, arq_csv_exportada); 
                            break;

                        case 0: 
                            break;

                        default: printf("Opção inválida!\n");
                    }
                } while (op_configs != 0);
                break;
                //case 3 do switch (op_navegar) 

            case 0:
                F_Salvar_Binario(arq_perguntas, questionario, *total_perguntas);
                printf("Dados salvos em '%s'\n", arq_bin);
                break;
                //case 0 do switch (op_navegar) 

            default:
                printf("Opção inválida!\n");
        }
        //switch (op_navegar) 

    } 
    while (op_navegar != 0);

    //ao finalizar retorna para o menu para liberar memória
    return questionario;
}



//>Main
int main()
{
    srand(time(NULL));
#ifdef _WIN32
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif

    FILE *arq_perguntas = NULL;
    Quiz *questionario = NULL;
    
    int total_perguntas = 0;


    //carrega arq bin ou importa csv na 1ª execução
    questionario = F_Inicializa_Sistema(&arq_perguntas, &total_perguntas);


    //!!Chama menu temporário!!
    questionario = F_Menu(questionario, &total_perguntas, &arq_perguntas);


    //libera memória
    free(questionario);
    

    return 0;
}