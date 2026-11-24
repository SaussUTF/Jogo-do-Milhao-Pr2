#if defined(_MSC_VER)
#pragma execution_character_set("utf-8")
#endif

#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "quiz.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 450

#define TOTAL_QUESTOES 15

const int NiveisporQuestao[TOTAL_QUESTOES] = {1, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5};
const int qtdNecessaria[6] = {0, 2, 2, 4, 4, 3};

//Bosses: o índice é o nível de dificuldade (1 = Muito Fácil ... 5 = Muito Difícil)
const char *BossNomes[6] = {"", "Luciano Huck", "Faustão", "Ana Maria Braga", "Celso", "Silvio Santos"};
const Color BossCores[6] = {
    {0, 0, 0, 255}, {40, 170, 90, 255}, {60, 130, 230, 255},
    {230, 120, 190, 255}, {240, 150, 40, 255}, {200, 50, 60, 255}
};

#define TOTAL_OPCOES 3
#define TOTAL_OPCOES_ITENS 3 //Volume da música, Gerenciamento de perguntas e voltar
#define PAINEL_WIDTH 300

#define LINHAS_VISIVEIS 6
#define LINHAS_HEIGHT 44

#define TOTAL_CAMPOS 7 //0 Enunciado | 1 - 4 Alternativas | 5 Correta | 6 Dificuldade

typedef enum{TELA_MENU, TELA_INTRO, TELA_JOGO, TELA_OPCOES, TELA_PERGUNTAS} Tela;

//Vidas do jogador
#define VIDAS_MAX 3
#define MOEDAS_POR_ACERTO 1
#define CUSTO_DICA 5

//Rodas da fortuna
typedef enum{RODA_NENHUMA, RODA_CURA, RODA_BOSS} TipoRoda;

//Como terminou a partida
typedef enum{FIM_JOGANDO, FIM_VENCEU, FIM_PERDEU} FimJogo;

//Em qual questão (0 a 14) cada roda aparece antes da pergunta.
//Cura: Ana Maria Braga (questão 7) e Celso (questão 12)
//Dano no boss (tira 2 vidas dele): só uma vez no Celso (questão 9) e uma vez no Silvio (questão 13)
const int RodaNaQuestao[TOTAL_QUESTOES] = {
    RODA_NENHUMA, RODA_NENHUMA, RODA_NENHUMA, RODA_NENHUMA,
    RODA_NENHUMA, RODA_NENHUMA, RODA_CURA,    RODA_NENHUMA,
    RODA_BOSS,    RODA_NENHUMA, RODA_NENHUMA, RODA_CURA,
    RODA_BOSS,    RODA_NENHUMA, RODA_NENHUMA
};

#define RODA_FATIAS 8
#define RODA_DANO_BOSS 2

// Fatias alternadas: prêmio, sem prêmio, prêmio, sem prêmio...
// 0 = dano em dobro; 1 = eliminar duas alternativas; 2 = vida completa; -1 = sem prêmio.
const int PremioPorFatia[RODA_FATIAS] = {0, -1, 1, -1, 2, -1, 0, -1};

//O que a tela de gerenciamento tá fazendo
typedef enum{MODO_LISTA, MODO_EDITAR, MODO_REMOVER} ModoGer;

//Recursos que são carregdos no programa
typedef struct {
    Music musicaMenu;
    Sound Somselecao;
    Font FonteMenu;
    Font FonteJogo;
    Texture2D logo;
    Texture2D FundoMenu;
    Texture2D FundoJogo;
    Texture2D bossFoto[6]; //Índice 1 a 5 = um boss por nível (id == 0 quando não há imagem)
}Recursos;

//Estado da partida em andamento
typedef struct {
    double tempoResposta; //Momento em que a resposta foi confirmada (animação do boss)
    int questao;
    int sorteada;
    int select;
    bool respondida;
    int escolha;
    int *usadas;
    bool podeJogar;
    char aviso[160];
    int vidas; //Vidas do jogador
    FimJogo fim; //FIM_JOGANDO enquanto a partida está rolando
    TipoRoda rodaTipo; //RODA_NENHUMA = sem roda na tela
    int rodaEstado; //0 = esperando ENTER para girar | 1 = girando | 2 = mostrando o resultado
    float rodaAngulo; //Ângulo atual da roda (graus)
    float rodaVel; //Velocidade de giro (graus por segundo)
    char rodaMsg[100]; //Texto do resultado da roda
    bool danoDobrado; //Próxima resposta correta causa dano dobrado
    bool alternativasEliminadas[4]; //Alternativas removidas por prêmio ou dica
    int moedas; //Moedas recebidas por respostas corretas
    bool dicaUsada; //Limita a uma dica comprada por pergunta
    char dicaMsg[100]; //Retorno visual da compra da dica
}Partida;

//Estado da tela de opções
typedef struct {
    int selecionado; //0 = volume | 1 = Gerecenciar perguntas | 2 = Voltar ao menu
    int volume; //Volume da música de 0 a 100
}Settings;

//Estado da tela de gerenciamento das perguntas
typedef struct {
    int selecionado; //posição da pergunta selecionada dentro do nivel atual
    int topo; //posição da primeira pergunta visivel na lista do nivel
    int nivel; //nivel exibido (1 a 5)
    ModoGer modo; //o que a tela tá fazendo agora
    int campo; //0 Enunciado | 1 - 4 Alternativas | 5 Correta | 6 Dificuldade
    int idx; //indice real do vetor (-1 = pergunta nova)
    Quiz rascunho; //cópia que está sendo editada
    char aviso[100]; //Mensagem de validação na edição
}GerQuestoes;

//Layout do menu
typedef struct {
    int PainelX;
    int PainelY;
    int Painelwidth;
    int Painelheigth;
    float LogoWidth;
    float LogoHeitgh;
    int InicioOP;
    Rectangle origem;
    Rectangle destino;
}Layout;

//Estado do menu -> Animações e seleção
typedef struct {
    int selecionado;
    float escala[TOTAL_OPCOES];
    float deslocamento[TOTAL_OPCOES];
    float pulso;
}Menu;

const char *MenuOP[TOTAL_OPCOES] = {"Jogar", "Opções", "Sair"};

//Função do Load de recursos
Recursos LoadRecursos(void){
    Recursos r;
    
    //Música do menu:
    r.musicaMenu = LoadMusicStream("recursos/Musicatema.wav");
    r.musicaMenu.looping = true;
    SetMusicVolume(r.musicaMenu, 0.8f);
    PlayMusicStream(r.musicaMenu);
    
    //Som de selecionar:
    r.Somselecao = LoadSound("recursos/Menuselect.mp3");
    SetSoundVolume(r.Somselecao, 0.7f);

    // Fonte Lato Regular: limpa, legível e com suporte aos acentos do português.
    // Coloque Lato-Regular.ttf dentro da pasta recursos do projeto.
    const char *fontesLato[] = {
        "recursos/Lato-Regular.ttf",
        "recursos/lato-regular.ttf",
        "Lato-Regular.ttf",
        "/usr/share/fonts/truetype/lato/Lato-Regular.ttf",
        "/Library/Fonts/Lato-Regular.ttf",
        "C:/Windows/Fonts/Lato-Regular.ttf"
    };
    const char *caminhoLato = NULL;
    for (int i = 0; i < (int)(sizeof(fontesLato) / sizeof(fontesLato[0])); i++){
        if (FileExists(fontesLato[i])){
            caminhoLato = fontesLato[i];
            break;
        }
    }

    // ASCII imprimível + Latin-1 (á, ã, ç, ê, õ etc.) + pontuação Unicode.
    int codepoints[95 + 96 + 9];
    int n = 0;
    for (int c = 32; c <= 126; c++) codepoints[n++] = c;
    for (int c = 160; c <= 255; c++) codepoints[n++] = c;
    const int extras[] = {0x20AC, 0x2013, 0x2014, 0x2018, 0x2019, 0x201C, 0x201D, 0x2026, 0x2022};
    for (int i = 0; i < 9; i++) codepoints[n++] = extras[i];

    if (caminhoLato != NULL){
        r.FonteMenu = LoadFontEx(caminhoLato, 64, codepoints, n);
        r.FonteJogo = LoadFontEx(caminhoLato, 64, codepoints, n);
        SetTextureFilter(r.FonteMenu.texture, TEXTURE_FILTER_BILINEAR);
        SetTextureFilter(r.FonteJogo.texture, TEXTURE_FILTER_BILINEAR);
    }else{
        TraceLog(LOG_ERROR, "Fonte Lato nao encontrada. Adicione recursos/Lato-Regular.ttf ao projeto.");
        r.FonteMenu = GetFontDefault();
        r.FonteJogo = GetFontDefault();
    }

    // Logo
    r.logo = LoadTexture("recursos/logo.png");
    SetTextureFilter(r.logo, TEXTURE_FILTER_BILINEAR);

    // Fundos: menu e jogo
    r.FundoMenu = LoadTexture("recursos/FundoJogo.png");
    SetTextureFilter(r.FundoMenu, TEXTURE_FILTER_BILINEAR);
    r.FundoJogo = LoadTexture("recursos/FundoJogo.png");
    SetTextureFilter(r.FundoJogo, TEXTURE_FILTER_BILINEAR);

    //Fotos dos bosses: recursos/boss1.png ... boss5.png (se não existir, usa um desenho padrão)
    r.bossFoto[0] = (Texture2D){0};
    for (int b = 1; b <= 5; b++){
        const char *caminho = TextFormat("recursos/boss%d.png", b);
        if (FileExists(caminho)){
            r.bossFoto[b] = LoadTexture(caminho);
            SetTextureFilter(r.bossFoto[b], TEXTURE_FILTER_BILINEAR);
        }else{
            r.bossFoto[b] = (Texture2D){0};
        }
    }

    return r;
}

//Função do Unload de recursos
void UnloadRecursos(Recursos *r){
    UnloadTexture(r->logo);
    UnloadTexture(r->FundoMenu);
    UnloadTexture(r->FundoJogo);
    for (int b = 1; b <= 5; b++){
        if (r->bossFoto[b].id != 0){
            UnloadTexture(r->bossFoto[b]);
        }
    }
    UnloadFont(r->FonteMenu);
    UnloadFont(r->FonteJogo);
    UnloadMusicStream(r->musicaMenu);
    UnloadSound(r->Somselecao);
}

//Inicialização do layout e do menu
Layout CalcularLayout(const Recursos *r){
    Layout l;

    //Dados do painel vermelho do menu:
    l.PainelX = 0;
    l.PainelY = 0;
    l.Painelwidth = PAINEL_WIDTH;
    l.Painelheigth = SCREEN_HEIGHT;

    //Dados do Logo
    l.LogoWidth = l.Painelwidth - 40;
    l.LogoHeitgh = l.LogoWidth * r->logo.height/r->logo.width;
    l.InicioOP = 40 + (int)l.LogoHeitgh + 40;
    l.origem = (Rectangle){0, 0, (float)l.LogoWidth, (float)l.LogoHeitgh};
    l.destino = (Rectangle){l.PainelX + (l.Painelwidth - l.LogoWidth)/2, 40, l.LogoWidth, l.LogoHeitgh};
    
    return l;
}

Menu criarMenu(void){
    Menu m;
    m.selecionado = 0;
    m.pulso = 1.0f;
    for (int i = 0; i < TOTAL_OPCOES; i++){
        m.escala[i] = 1.0f;
        m.deslocamento[i] = 0.0f;
    }
    
    return m;
}

//Atualização do menu
void atualMenu(Menu *m, Tela *telaAtual, bool *sair, const Recursos *r){
    //Navegação entre as opções
    if (IsKeyPressed(KEY_DOWN)){
        m->selecionado++;
        if (m->selecionado >= TOTAL_OPCOES){
            m->selecionado = 0;
        }
    }
    if (IsKeyPressed(KEY_UP)){
        m->selecionado--;
        if (m->selecionado < 0){
            m->selecionado = TOTAL_OPCOES - 1;
        }
    }
    
    //Animação das opções
    float dt = GetFrameTime();
    for (int i = 0; i < TOTAL_OPCOES; i++){
        float alvoEscala = (i == m->selecionado) ? 1.3f : 1.0f;
        float alvoDesloc = (i == m->selecionado) ? 20.0f : 0.0f;

        m->escala[i] += (alvoEscala - m->escala[i]) * 10.0f * dt; 
        m->deslocamento[i] += (alvoDesloc - m->deslocamento[i]) * 10.0f * dt;
    }
    m->pulso = 1.0f + sinf((float)GetTime() * 5.0f) * 0.05f;

    //Confirmar a opção
    if (IsKeyPressed(KEY_ENTER)){
        if (m->selecionado == 0){
            *telaAtual = TELA_INTRO;
            PlaySound(r->Somselecao);
        }
        if (m->selecionado == 1){
            *telaAtual = TELA_OPCOES;
            PlaySound(r->Somselecao);
        }
        if (m->selecionado == 2){
            *sair = true;
        }
        
    }
    
}

static int ContarLivres(const Partida *partida, const Quiz *perguntas, int total, int nivel){
    int cont = 0;
    for (int i = 0; i < total; i++){
        if (perguntas[i].dificuldade == nivel && partida->usadas[i] == 0){
            cont++;
        }
    }
    return cont;
}

void sortearPergunta(Partida *partida, const Quiz *perguntas, int total){
    int nivel = NiveisporQuestao[partida->questao];
    int atual = partida->sorteada;
    int livres = ContarLivres(partida, perguntas, total, nivel);

    //Acabaram as perguntas inéditas do nível (acontece quando o jogador erra e tenta de novo):
    //libera as outras do nível, menos a que acabou de sair
    if (livres == 0){
        for (int i = 0; i < total; i++){
            if (perguntas[i].dificuldade == nivel && i != atual){
                partida->usadas[i] = 0;
            }
        }
        livres = ContarLivres(partida, perguntas, total, nivel);
    }
    //Só existe uma pergunta no nível: não tem jeito, repete
    if (livres == 0){
        partida->usadas[atual] = 0;
        livres = 1;
    }

    int alvo = rand() % livres;
    for (int i = 0; i < total; i++){
        if (perguntas[i].dificuldade == nivel && partida->usadas[i] == 0){
            if (alvo == 0){
                partida->sorteada = i;
                partida->usadas[i] = 1;
                return;
            }
            alvo--;
        }
    }
}

//Liga a Roda da Sorte se a questão atual tiver uma
void IniciarRodaSeHouver(Partida *partida){
    TipoRoda tipo = (TipoRoda)RodaNaQuestao[partida->questao];
    if (tipo == RODA_NENHUMA){
        return;
    }
    partida->rodaTipo = tipo;
    partida->rodaEstado = 0;
    partida->rodaAngulo = (float)(rand() % 360);
    partida->rodaVel = 0.0f;
    partida->rodaMsg[0] = '\0';
}

//Aplica o prêmio indicado pela fatia em que a roda parou
void AplicarPremioRoda(Partida *partida, const Quiz *perguntas, int total){
    (void)perguntas;
    (void)total;

    float angulo = fmodf(270.0f - partida->rodaAngulo, 360.0f);
    if (angulo < 0.0f) angulo += 360.0f;
    int fatia = (int)(angulo / (360.0f / RODA_FATIAS));
    // 4/8 fatias premiadas (50%) e 4/8 sem prêmio (50%).
    // A distribuição mantém os três benefícios já existentes.
    int premio = PremioPorFatia[fatia];

    if (premio == -1){
        snprintf(partida->rodaMsg, sizeof(partida->rodaMsg), "Não foi dessa vez! Você não ganhou nada.");
    }else if (premio == 0){
        partida->danoDobrado = true;
        snprintf(partida->rodaMsg, sizeof(partida->rodaMsg), "DANO EM DOBRO na próxima resposta correta!");
    }else if (premio == 1){
        //Escolhe duas alternativas incorretas para retirar
        int correta = perguntas[partida->sorteada].alternativa_correta - 'A';
        int candidatas[3];
        int n = 0;
        for (int i = 0; i < 4; i++){
            if (i != correta) candidatas[n++] = i;
        }
        int primeira = rand() % n;
        int segunda = (primeira + 1 + rand() % (n - 1)) % n;
        partida->alternativasEliminadas[candidatas[primeira]] = true;
        partida->alternativasEliminadas[candidatas[segunda]] = true;
        partida->select = correta;
        snprintf(partida->rodaMsg, sizeof(partida->rodaMsg), "Duas alternativas incorretas foram eliminadas!");
    }else{
        partida->vidas = VIDAS_MAX;
        snprintf(partida->rodaMsg, sizeof(partida->rodaMsg), "Você recuperou todas as suas vidas!");
    }
}

//Compra uma dica por 5 moedas: remove uma alternativa incorreta ainda disponível.
//A dica só pode ser comprada uma vez por pergunta.
void ComprarDica(Partida *partida, const Quiz *perguntas){
    if (partida->respondida){
        return;
    }
    if (partida->dicaUsada){
        snprintf(partida->dicaMsg, sizeof(partida->dicaMsg), "Você já usou a ajuda nesta pergunta.");
        return;
    }
    if (partida->moedas < CUSTO_DICA){
        snprintf(partida->dicaMsg, sizeof(partida->dicaMsg), "Moedas insuficientes: junte %d moedas para a ajuda.", CUSTO_DICA);
        return;
    }

    int correta = perguntas[partida->sorteada].alternativa_correta - 'A';
    int candidatas[4];
    int n = 0;
    for (int i = 0; i < 4; i++){
        if (i != correta && !partida->alternativasEliminadas[i]){
            candidatas[n++] = i;
        }
    }
    if (n == 0){
        snprintf(partida->dicaMsg, sizeof(partida->dicaMsg), "Não há outra alternativa para eliminar.");
        return;
    }

    int removida = candidatas[rand() % n];
    partida->alternativasEliminadas[removida] = true;
    partida->moedas -= CUSTO_DICA;
    partida->dicaUsada = true;

    // Se a opção selecionada foi removida, move a seleção para uma alternativa válida.
    if (partida->select == removida){
        for (int i = 0; i < 4; i++){
            if (!partida->alternativasEliminadas[i]){
                partida->select = i;
                break;
            }
        }
    }
    snprintf(partida->dicaMsg, sizeof(partida->dicaMsg), "Ajuda comprada: uma alternativa incorreta foi eliminada!");
}

//Atualiza a roda: ENTER gira, ela freia até parar e ENTER fecha
void AtualizarRoda(Partida *partida, const Quiz *perguntas, int total, const Recursos *r){
    if (partida->rodaEstado == 0){
        if (IsKeyPressed(KEY_ENTER)){
            partida->rodaEstado = 1;
            partida->rodaVel = 900.0f + (float)(rand() % 300);
            PlaySound(r->Somselecao);
        }
    }else if (partida->rodaEstado == 1){
        float dt = GetFrameTime();
        partida->rodaAngulo += partida->rodaVel * dt;
        partida->rodaAngulo = fmodf(partida->rodaAngulo, 360.0f);
        partida->rodaVel -= 420.0f * dt; //Desaceleração constante para a roda sempre parar
        if (partida->rodaVel <= 0.0f){
            partida->rodaVel = 0.0f;
            partida->rodaEstado = 2;
            AplicarPremioRoda(partida, perguntas, total);
        }
    }else if (IsKeyPressed(KEY_ENTER)){
        partida->rodaTipo = RODA_NENHUMA;
        //Não reabre a roda na mesma pergunta. A próxima abertura acontece
        //normalmente quando o jogador avançar para outra questão.
    }
}

void IniciarPartida(Partida *partida, const Quiz *perguntas, int total){
    free(partida->usadas);
    *partida = (Partida){0};
    partida->podeJogar = true;
    partida->vidas = VIDAS_MAX;

    for (int niv = 1; niv <= 5; niv++){
        int cont = 0;
        for (int i = 0; i < total; i++){
            if (perguntas[i].dificuldade == niv){
                cont++;
            }
            
        }
        if (cont < qtdNecessaria[niv]){
                partida->podeJogar = false;
                snprintf(partida->aviso, sizeof(partida->aviso), "O nível %s precisa ter pelo menos %d perguntas (atual: %d)", F_Nome_Dificuldade(niv), qtdNecessaria[niv], cont);
                return;
        }
    }
    


    partida->usadas = (int *)calloc(total, sizeof(int));
    if (partida->usadas == NULL){
        partida->podeJogar = false;
        snprintf(partida->aviso, sizeof(partida->aviso), "Erro ao alocar memória!");
        TraceLog(LOG_WARNING, "IniciarPartida: %s", partida->aviso);
        return;
    }
    
    sortearPergunta(partida, perguntas, total);
    TraceLog(LOG_INFO, "IniciarPartida ok: podeJogar=%d questao=%d sorteada=%d", partida->podeJogar, partida->questao, partida->sorteada);
}

void VoltarAoMenu(Tela *telaAtual, const Recursos *r){
    *telaAtual = TELA_MENU;
    ResumeMusicStream(r->musicaMenu);
}

//Tela de introdução: mostra o que o jogador vai enfrentar
void atualIntro(Tela *telaAtual, const Recursos *r){
    if (IsKeyPressed(KEY_ENTER)){
        PlaySound(r->Somselecao);
        PauseMusicStream(r->musicaMenu);
        *telaAtual = TELA_JOGO;
    }
    if (IsKeyPressed(KEY_ESCAPE)){
        *telaAtual = TELA_MENU;
    }
}

// Quantas vidas o boss atual ainda tem antes da pergunta atual.
// Cada pergunta do nível representa uma vida do respectivo boss.
int VidasBossNaQuestao(int questao){
    if (questao < 0 || questao >= TOTAL_QUESTOES) return 0;
    int nivel = NiveisporQuestao[questao];
    int vidas = 0;
    for (int i = questao; i < TOTAL_QUESTOES; i++){
        if (NiveisporQuestao[i] == nivel) vidas++;
    }
    return vidas;
}

void atualJogo(Tela *telaAtual, const Recursos *r, Partida *partida, const Quiz *perguntas, int total){
    
    if (!partida->podeJogar){
        if (IsKeyPressed(KEY_ENTER)){//Não para jogar: ENTER e volta ao menu
            VoltarAoMenu(telaAtual, r);
        }
        
    }else if (partida->rodaTipo != RODA_NENHUMA){
        AtualizarRoda(partida, perguntas, total, r);

    }else if (partida->fim != FIM_JOGANDO){
        if (IsKeyPressed(KEY_ENTER)){//Venceu ou perdeu: ENTER volta ao menu
            VoltarAoMenu(telaAtual, r);
        }

    }else if (!partida->respondida){
        if (IsKeyPressed(KEY_H)){
            ComprarDica(partida, perguntas);
        }
        if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_UP)){
            int direcao = IsKeyPressed(KEY_DOWN) ? 1 : -1;
            for (int tentativas = 0; tentativas < 4; tentativas++){
                partida->select = (partida->select + direcao + 4) % 4;
                if (!partida->alternativasEliminadas[partida->select]) break;
            }
        }
        if (IsKeyPressed(KEY_ENTER) && !partida->alternativasEliminadas[partida->select]){
            partida->escolha = partida->select;
            partida->respondida = true;
            partida->tempoResposta = GetTime();
            PlaySound(r->Somselecao);

            // Na última vida do boss, ele fica enfurecido e causa dano dobrado.
            if ('A' + partida->escolha != perguntas[partida->sorteada].alternativa_correta){
                int danoBoss = (VidasBossNaQuestao(partida->questao) == 1) ? 2 : 1;
                partida->vidas -= danoBoss;
                if (partida->vidas < 0) partida->vidas = 0;
            }else{
                // Cada resposta correta concede uma moeda, já durante a tela de resultado.
                partida->moedas += MOEDAS_POR_ACERTO;
            }
        }

    }else if (IsKeyPressed(KEY_ENTER)){
        //Já respondeu: ENTER avança
        bool Acertou = ('A' + partida->escolha == perguntas[partida->sorteada].alternativa_correta);

        if (Acertou){
            if (partida->questao == TOTAL_QUESTOES - 1){
                partida->fim = FIM_VENCEU;
            }else{
                int nivelAtual = NiveisporQuestao[partida->questao];
                int dano = partida->danoDobrado ? 2 : 1;
                partida->danoDobrado = false;
                partida->questao++;
                if (dano == 2 && partida->questao < TOTAL_QUESTOES &&
                    NiveisporQuestao[partida->questao] == nivelAtual){
                    partida->questao++;
                }
                if (partida->questao >= TOTAL_QUESTOES){
                    partida->fim = FIM_VENCEU;
                }else{
                    sortearPergunta(partida, perguntas, total);
                    partida->select = 0;
                    partida->respondida = false;
                    memset(partida->alternativasEliminadas, 0, sizeof(partida->alternativasEliminadas));
                    partida->dicaUsada = false;
                    partida->dicaMsg[0] = '\0';
                    IniciarRodaSeHouver(partida);
                }
            }
        }else{
            if (partida->vidas <= 0){
                partida->fim = FIM_PERDEU;
            }else{
                //Ainda tem vida: tenta outra pergunta do mesmo nível (o boss continua com a mesma vida)
                sortearPergunta(partida, perguntas, total);
                partida->select = 0;
                partida->respondida = false;
                memset(partida->alternativasEliminadas, 0, sizeof(partida->alternativasEliminadas));
                partida->dicaUsada = false;
                partida->dicaMsg[0] = '\0';
            }
        }
        
    }
    
    if (IsKeyPressed(KEY_ESCAPE)){
        VoltarAoMenu(telaAtual, r);
    }
}

void atualOpcoes(Tela *telaAtual, const Recursos *r, Settings *op){
    //Navegação entre os itens
    if (IsKeyPressed(KEY_DOWN)){
        op->selecionado++;
        if (op->selecionado >= TOTAL_OPCOES_ITENS){
            op->selecionado = 0;
        }
    }
    if (IsKeyPressed(KEY_UP)){
        op->selecionado--;
        if (op->selecionado < 0){
            op->selecionado = TOTAL_OPCOES_ITENS - 1;
        }
    }
    //Volume: as setas esquerda e direita ajustam o volume de 10 em 10 
    if (op->selecionado == 0){
        if (IsKeyPressed(KEY_RIGHT) && op->volume < 100){
            op->volume += 10;
            SetMusicVolume(r->musicaMenu, op->volume / 100.0f);
        }
        
        if (IsKeyPressed(KEY_LEFT) && op->volume > 0){
            op->volume -= 10;
            SetMusicVolume(r->musicaMenu, op->volume / 100.0f);
        }
    }
    
    if (IsKeyPressed(KEY_ENTER) && op->selecionado == 1){
        *telaAtual = TELA_PERGUNTAS;
        PlaySound(r->Somselecao);
    }
    
    if (IsKeyPressed(KEY_ENTER) && op->selecionado == 2){
        *telaAtual = TELA_MENU;
    }
}

//Desenhos da tela: Menu, Opções e Jogo
void DesenhoMenu(const Menu *m, const Recursos *r, const Layout *l){
    ClearBackground(BLACK);
    
    //Fundo do show do milhão
    DrawTexturePro(r->FundoMenu, 
    (Rectangle){0, 0, (float)r->FundoMenu.width, (float)r->FundoMenu.height},
    (Rectangle){0, 0, SCREEN_WIDTH, SCREEN_HEIGHT},
    (Vector2){0, 0}, 0.0f, WHITE);
    
    //Painel vermelho do menu
    DrawRectangle(l->PainelX, l->PainelY, l->Painelwidth, l->Painelheigth, RED);

    //Logo do show do milhão
    DrawTexturePro(r->logo, l->origem, l->destino, (Vector2){0, 0}, 0.0f, WHITE);

    //Opções que tem no menu
    for (int i = 0; i < TOTAL_OPCOES; i++){
        float fator = m->escala[i] * ((i == m->selecionado) ? m->pulso : 1.0f);
        int tamFonte = (int)(30 * fator);
        Vector2 tam = MeasureTextEx(r->FonteMenu, MenuOP[i], tamFonte, 2);

        float X = l->PainelX + (l->Painelwidth - tam.x) / 2 + (int)m->deslocamento[i];
        float Y = l->InicioOP + i * 50 - (tamFonte - 30)/2;

        Color cor = (i == m->selecionado) ? YELLOW : WHITE;
        DrawTextEx(r->FonteMenu, MenuOP[i], (Vector2){X, Y}, 30, 2, cor);
    }
}

float Textoquebrado(Font fonte, const char *texto, float x, float y, float WidthMAX, float tam, Color cor, bool desenhar){
    char linha[512] = "";
    char palavra[256];
    char teste[768];
    float linhaHeight = tam * 1.2f;
    const char *p = texto;

    while (*p){
        //Pula os espaços
        while (*p == ' '){
            p++;
        }
        if (*p == '\0'){
            break;
        }

        //Pega a próxima palavra
        int n = 0;
        while (p[n] != '\0' && p[n] != ' ' && n < 255){
            n++;
        }
        memcpy(palavra, p, n);
        palavra[n] = '\0';
        p += n;

        //Testa se a palavra cabe na linha atual
        if (linha[0] != '\0'){
            snprintf(teste, sizeof(teste), "%s %s", linha, palavra);
        }else{
            snprintf(teste, sizeof(teste), "%s", palavra);
        }
        
        if (linha[0] != '\0' && MeasureTextEx(fonte, teste, tam, 1).x > WidthMAX){
            //Não coube: desenha a linha pronta e começa outra com essa palavra
            if (desenhar){
                DrawTextEx(fonte, linha, (Vector2){x, y}, tam, 1, cor);
            }
            
            y += linhaHeight;
            snprintf(linha, sizeof(linha), "%s", palavra); 
        }else{
            size_t len = strlen(teste);
            if (len >= sizeof(linha)){
                len = sizeof(linha) - 1;
            }
            memcpy(linha, teste, len);
                linha[len] = '\0';
        }
    }

    if (linha[0] != '\0'){
        if (desenhar){
            DrawTextEx(fonte, linha, (Vector2){x, y}, tam, 1, cor);
        }
        y += linhaHeight;
    }
    
    return y;
}

void TextoCentro(Font fonte, const char *texto, float y, float tamanho, Color cor){
    Vector2 tam = MeasureTextEx(fonte, texto, tamanho, 1);
    DrawTextEx(fonte, texto, (Vector2){(SCREEN_WIDTH - tam.x) / 2, y}, tamanho, 1, cor);
}

//Painel do boss (metade esquerda): retrato, nome e vida (uma barrinha por pergunta do nível)
void DesenharBoss(const Recursos *r, Rectangle area, int nivel, const Partida *partida, bool acertou){
    Color cor = BossCores[nivel];

    // Cada pergunta do nível representa uma vida do boss.
    int total = 0;
    for (int i = 0; i < TOTAL_QUESTOES; i++){
        if (NiveisporQuestao[i] == nivel) total++;
    }
    int vida = VidasBossNaQuestao(partida->questao);
    if (partida->respondida && acertou) vida--;
    if (vida < 0) vida = 0;
    bool bossEnfurecido = (VidasBossNaQuestao(partida->questao) == 1) &&
                          !(partida->respondida && acertou);

    // Moldura pulsante em vermelho somente durante a fase enfurecida.
    float pulsoFuria = 0.5f + 0.5f * sinf((float)GetTime() * 5.0f);
    DrawRectangleRounded((Rectangle){area.x - 3, area.y - 3, area.width + 6, area.height + 6},
                         0.10f, 16, bossEnfurecido ? Fade(RED, 0.72f + pulsoFuria * 0.22f) : Fade(cor, 0.95f));
    DrawRectangleRounded(area, 0.10f, 16, (Color){8, 15, 38, 242});
    DrawRectangleRoundedLinesEx(area, 0.10f, 16, 2.0f,
                                bossEnfurecido ? Fade(RED, 0.55f + pulsoFuria * 0.35f) : Fade(WHITE, 0.22f));

    //Área do retrato
    Rectangle retrato = {area.x + 10, area.y + 10, area.width - 20, area.height - 74};

    //Tremida e flash quando o jogador acerta (o boss leva o golpe)
    float tempo = (float)(GetTime() - partida->tempoResposta);
    float tremida = 0.0f;
    float flash = 0.0f;
    if (partida->respondida && tempo < 0.65f){
        float intensidade = 1.0f - tempo / 0.65f;
        // Acerto: boss recebe o golpe; erro: boss contra-ataca visualmente.
        tremida = sinf(tempo * (acertou ? 62.0f : 42.0f)) * (acertou ? 7.0f : 4.0f) * intensidade;
        flash = (acertou ? 0.42f : 0.28f) * intensidade;
    }
    Rectangle destino = {retrato.x + tremida, retrato.y, retrato.width, retrato.height};

    if (r->bossFoto[nivel].id != 0){
        //Foto: preenche o retrato cortando o excesso (mantém o topo, onde fica o rosto)
        Texture2D foto = r->bossFoto[nivel];
        float esc = fmaxf(destino.width / foto.width, destino.height / foto.height);
        float srcW = destino.width / esc;
        float srcH = destino.height / esc;
        Rectangle origem = {(foto.width - srcW) / 2, (foto.height - srcH) * 0.15f, srcW, srcH};
        DrawTexturePro(foto, origem, destino, (Vector2){0, 0}, 0.0f, WHITE);
    }else{
        //Desenho padrão enquanto não houver foto
        DrawRectangleRounded(destino, 0.08f, 12, Fade(cor, 0.35f));
        float cx = destino.x + destino.width / 2;
        float base = destino.y + destino.height;
        DrawEllipse((int)cx, (int)(base - 4), destino.width * 0.30f, destino.height * 0.28f, Fade(cor, 0.85f));
        DrawCircle((int)cx, (int)(destino.y + destino.height * 0.38f), destino.height * 0.17f, Fade(cor, 0.85f));
        const char *dica = TextFormat("coloque recursos/boss%d.png", nivel);
        Vector2 td = MeasureTextEx(r->FonteJogo, dica, 12, 1);
        DrawTextEx(r->FonteJogo, dica, (Vector2){destino.x + (destino.width - td.x) / 2, destino.y + 6}, 12, 1, LIGHTGRAY);
    }
    DrawRectangleLinesEx(destino, 2, cor);

    if (flash > 0.0f){
        DrawRectangleRec(destino, Fade(acertou ? GOLD : RED, flash));
    }

    // A fúria começa exclusivamente quando resta a última vida do boss.
    if (bossEnfurecido){
        DrawRectangleRounded((Rectangle){area.x + 8, area.y + 7, area.width - 16, 22},
                             0.5f, 8, Fade((Color){190, 28, 42, 255}, 0.82f + pulsoFuria * 0.16f));
        const char *fase = "BOSS ENFURECIDO | DANO DUPLO";
        Vector2 tf = MeasureTextEx(r->FonteJogo, fase, 13, 1);
        DrawTextEx(r->FonteJogo, fase,
                   (Vector2){area.x + (area.width - tf.x) / 2, area.y + 11}, 13, 1, WHITE);
    }

    //Boss derrotado
    if (vida == 0 && partida->respondida && acertou){
        DrawRectangleRec(destino, Fade(BLACK, 0.55f));
        Vector2 td = MeasureTextEx(r->FonteJogo, "DERROTADO!", 30, 1);
        DrawTextEx(r->FonteJogo, "DERROTADO!", (Vector2){destino.x + (destino.width - td.x) / 2, destino.y + (destino.height - td.y) / 2}, 30, 1, GOLD);
    }

    //Nome
    Vector2 tn = MeasureTextEx(r->FonteJogo, BossNomes[nivel], 24, 1);
    DrawTextEx(r->FonteJogo, BossNomes[nivel], (Vector2){area.x + (area.width - tn.x) / 2, retrato.y + retrato.height + 6}, 24, 1, WHITE);

    //Barra de vida em segmentos
    float margem = 14.0f;
    float espaco = 5.0f;
    float segW = (area.width - 2 * margem - espaco * (total - 1)) / total;
    float segY = area.y + area.height - 24;
    for (int i = 0; i < total; i++){
        Rectangle seg = {area.x + margem + i * (segW + espaco), segY, segW, 12};
        DrawRectangleRounded(seg, 0.5f, 6, i < vida ? (Color){220, 40, 50, 255} : (Color){60, 30, 40, 255});
    }
}

//Coração simples feito com dois círculos e um triângulo
void DesenharCoracao(float cx, float cy, float tam, Color cor){
    DrawCircle((int)(cx - tam * 0.25f), (int)(cy - tam * 0.12f), tam * 0.28f, cor);
    DrawCircle((int)(cx + tam * 0.25f), (int)(cy - tam * 0.12f), tam * 0.28f, cor);
    DrawTriangle((Vector2){cx - tam * 0.51f, cy - tam * 0.02f}, (Vector2){cx, cy + tam * 0.5f}, (Vector2){cx + tam * 0.51f, cy - tam * 0.02f}, cor);
}

//Vidas do jogador no canto superior direito. Retorna a largura ocupada
float DesenharVidas(const Partida *partida){
    const float tam = 22.0f;
    const float passo = 28.0f;
    float larg = VIDAS_MAX * passo;
    float x0 = SCREEN_WIDTH - 20 - larg;

    for (int i = 0; i < VIDAS_MAX; i++){
        Color cor = (i < partida->vidas) ? (Color){230, 40, 60, 255} : (Color){70, 40, 55, 255};
        DesenharCoracao(x0 + i * passo + passo / 2, 22, tam, cor);
    }
    return larg;
}

//Tela de introdução: o que o jogador vai enfrentar
void DesenhoIntro(const Recursos *r){
    ClearBackground(DARKGRAY);

    DrawTexturePro(r->FundoMenu,
    (Rectangle){0, 0, (float)r->FundoMenu.width, (float)r->FundoMenu.height},
    (Rectangle){0, 0, SCREEN_WIDTH, SCREEN_HEIGHT}, (Vector2){0, 0}, 0.0f, WHITE);
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.65f));

    TextoCentro(r->FonteJogo, "O QUE VOCÊ VAI ENFRENTAR", 10, 30, WHITE);
    TextoCentro(r->FonteJogo, TextFormat("%d perguntas, 5 bosses e 5 níveis de dificuldade. Cada boss tem uma barra de vida!", TOTAL_QUESTOES), 48, 15, YELLOW);

    for (int niv = 1; niv <= 5; niv++){
        int perg = 0;
        for (int i = 0; i < TOTAL_QUESTOES; i++){
            if (NiveisporQuestao[i] == niv){
                perg++;
            }
        }

        Rectangle caixa = {40, 76 + (niv - 1) * 50, 720, 44};
        DrawRectangleRec(caixa, (Color){15, 25, 110, 235});
        DrawRectangleLinesEx(caixa, 2, BossCores[niv]);
        DrawRectangle((int)caixa.x, (int)caixa.y, 10, (int)caixa.height, BossCores[niv]);

        //Número do boss
        Vector2 centro = {caixa.x + 36, caixa.y + caixa.height / 2};
        DrawCircleV(centro, 15, BossCores[niv]);
        const char *num = TextFormat("%d", niv);
        Vector2 tn = MeasureTextEx(r->FonteJogo, num, 20, 1);
        DrawTextEx(r->FonteJogo, num, (Vector2){centro.x - tn.x / 2, centro.y - tn.y / 2}, 20, 1, WHITE);

        //Nome do boss e nível de dificuldade
        DrawTextEx(r->FonteJogo, BossNomes[niv], (Vector2){caixa.x + 64, caixa.y + 10}, 24, 1, WHITE);
        const char *nivel = TextFormat("Nível: %s", F_Nome_Dificuldade(niv));
        DrawTextEx(r->FonteJogo, nivel, (Vector2){caixa.x + 330, caixa.y + 14}, 18, 1, BossCores[niv]);

        //Quantas perguntas (= vidas do boss)
        const char *vidas = TextFormat("%d %s", perg, perg == 1 ? "pergunta" : "perguntas");
        Vector2 tv = MeasureTextEx(r->FonteJogo, vidas, 18, 1);
        DrawTextEx(r->FonteJogo, vidas, (Vector2){caixa.x + caixa.width - 16 - tv.x, caixa.y + 14}, 18, 1, LIGHTGRAY);
    }

    float y = 332;
    y = Textoquebrado(r->FonteJogo, TextFormat("Você começa com %d vidas. Cada erro tira vida; quando o boss chega à última vida, fica enfurecido e causa dano dobrado.", VIDAS_MAX), 40, y, 720, 15, WHITE, true);
    y = Textoquebrado(r->FonteJogo, "Cada acerto rende 1 moeda. Junte 5 moedas e use H para comprar ajuda: elimina uma alternativa incorreta (uma vez por pergunta).", 40, y + 2, 720, 15, (Color){165, 225, 240, 255}, true);
    y = Textoquebrado(r->FonteJogo, "A Roda da Sorte tem fatias alternadas: prêmio, nada, prêmio, nada. Metade das fatias não dá prêmio!", 40, y + 2, 720, 15, GOLD, true);

    TextoCentro(r->FonteJogo, "ENTER para começar | ESC para voltar ao menu", 422, 14, LIGHTGRAY);
}

//Roda da Sorte por cima da tela do jogo
void DesenhoRoda(const Recursos *r, const Partida *partida){
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.8f));

    TextoCentro(r->FonteJogo, "RODA DA SORTE", 12, 32, GOLD);
    TextoCentro(r->FonteJogo, "4 prêmios e 4 fatias sem prêmio: 50% / 50%", 52, 15, WHITE);

    Vector2 centro = {SCREEN_WIDTH / 2.0f, 225};
    float raio = 105.0f;
    float fatia = 360.0f / RODA_FATIAS;
    const char *rotulos[4] = {"2X DANO", "ELIMINAR 2", "VIDA MAX", "NADA"};
    const Color cores[4] = {
        (Color){205, 125, 35, 255},
        (Color){45, 125, 205, 255},
        (Color){35, 155, 105, 255},
        (Color){65, 70, 90, 255}
    };

    for (int i = 0; i < RODA_FATIAS; i++){
        float ini = partida->rodaAngulo + i * fatia;
        int premio = PremioPorFatia[i];
        int tipoVisual = premio < 0 ? 3 : premio;
        DrawCircleSector(centro, raio, ini, ini + fatia, 32, cores[tipoVisual]);
        DrawCircleSectorLines(centro, raio, ini, ini + fatia, 32, (Color){235, 235, 245, 220});
        float meio = (ini + fatia / 2.0f) * DEG2RAD;
        Vector2 pos = {centro.x + cosf(meio) * 69.0f, centro.y + sinf(meio) * 69.0f};
        Vector2 tam = MeasureTextEx(r->FonteJogo, rotulos[tipoVisual], 9, 1);
        DrawTextEx(r->FonteJogo, rotulos[tipoVisual], (Vector2){pos.x - tam.x / 2, pos.y - tam.y / 2}, 9, 1, WHITE);
    }
    DrawRing(centro, raio - 1, raio + 6, 0, 360, 64, WHITE);

    DrawCircleV(centro, 26, (Color){15, 25, 110, 255});
    DrawTextEx(r->FonteJogo, "?", (Vector2){centro.x - 8, centro.y - 15}, 28, 1, WHITE);

    //Ponteiro no topo
    float py = centro.y - raio - 26;
    DrawTriangle((Vector2){centro.x - 14, py}, (Vector2){centro.x, py + 34}, (Vector2){centro.x + 14, py}, WHITE);

    if (partida->rodaEstado == 0){
        TextoCentro(r->FonteJogo, "Pressione ENTER para girar", 360, 22, WHITE);
    }else if (partida->rodaEstado == 1){
        TextoCentro(r->FonteJogo, "Girando...", 360, 22, YELLOW);
    }else{
        TextoCentro(r->FonteJogo, partida->rodaMsg, 352, 20, GOLD);
        TextoCentro(r->FonteJogo, "ENTER para continuar", 392, 16, LIGHTGRAY);
    }
}

void DesenhoJogo(const Recursos *r, const Quiz *perguntas, int total, const Partida *partida){
    ClearBackground(DARKBLUE);

    DrawTexturePro(r->FundoJogo,
    (Rectangle){0, 0, (float)r->FundoJogo.width, (float)r->FundoJogo.height},
    (Rectangle){0, 0, SCREEN_WIDTH, SCREEN_HEIGHT},
    (Vector2){0, 0}, 0.0f, WHITE);
    // Camada escura translúcida para melhorar contraste e leitura.
    DrawRectangleGradientV(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
        (Color){5, 12, 34, 155}, (Color){8, 18, 48, 205});
    
    if (total == 0){
        DrawTextEx(r->FonteJogo, "Nenhuma pergunta carregada!", (Vector2){20, 20}, 24, 1, WHITE);
        return;
    }
    
    const Quiz *p = &perguntas[partida->sorteada]; //Por enquanto sempre a primeira pergunta
    char texto[200];
    
    snprintf(texto, sizeof(texto), "QUESTÃO %02d/%02d  •  %s", partida->questao + 1,
             TOTAL_QUESTOES, F_Nome_Dificuldade(p->dificuldade));
    DrawTextEx(r->FonteJogo, texto, (Vector2){20, 12}, 17, 1, (Color){235, 241, 250, 255});

    DesenharVidas(partida);

    if (partida->questao == TOTAL_QUESTOES - 1){
        Vector2 tamCabecalho = MeasureTextEx(r->FonteJogo, texto, 17, 1);
        Rectangle finalHud = {20 + tamCabecalho.x + 10, 7, 58, 29};
        DrawRectangleRounded(finalHud, 0.38f, 8, (Color){67, 42, 20, 245});
        DrawRectangleRoundedLinesEx(finalHud, 0.38f, 8, 1.0f, (Color){230, 158, 76, 230});
        DrawTextEx(r->FonteJogo, "FINAL", (Vector2){finalHud.x + 12, 14}, 12, 1, ORANGE);
    }
    
    // Cartão do enunciado: hierarquia visual clara e espaço confortável.
    Rectangle cartaoPergunta = {14, 34, 772, 92};
    DrawRectangleRounded((Rectangle){cartaoPergunta.x, cartaoPergunta.y + 4,
                                     cartaoPergunta.width, cartaoPergunta.height},
                        0.10f, 16, Fade(BLACK, 0.28f));
    DrawRectangleRounded(cartaoPergunta, 0.10f, 16, (Color){10, 20, 43, 238});
    DrawRectangleRoundedLinesEx(cartaoPergunta, 0.10f, 16, 1.5f, (Color){105, 145, 195, 155});
    DrawRectangleRounded((Rectangle){22, 45, 4, 70}, 0.5f, 6, (Color){65, 190, 220, 255});

    float tamEnun = 26;
    float larguraEnun = 736;
    float alturaEnun = Textoquebrado(r->FonteJogo, p->enunciado, 0, 0, larguraEnun, tamEnun, WHITE, false);
    while (alturaEnun > 68 && tamEnun > 16){
        tamEnun -= 1;
        alturaEnun = Textoquebrado(r->FonteJogo, p->enunciado, 0, 0, larguraEnun, tamEnun, WHITE, false);
    }
    float yEnun = cartaoPergunta.y + (cartaoPergunta.height - alturaEnun) / 2.0f;
    Textoquebrado(r->FonteJogo, p->enunciado, 34, yEnun, larguraEnun, tamEnun,
                  (Color){245, 248, 255, 255}, true);

    // Região inferior: painel do boss e quatro cartões de resposta.
    const float topo = 140.0f;
    const float fundoRegiao = 382.0f;
    const float alturaRegiao = fundoRegiao - topo;

    DesenharBoss(r, (Rectangle){20, topo, 370, alturaRegiao}, p->dificuldade, partida,
                 partida->respondida && ('A' + partida->escolha == p->alternativa_correta));

    const float colX = 410.0f;
    const float colW = 370.0f;
    const float gap = 8.0f;
    const float larguraTexto = colW - 64.0f;

    // Calcula a altura dos cartões para acomodar alternativas com mais texto.
    float minAlt = (alturaRegiao - 3.0f * gap) / 4.0f;
    if (minAlt > 64.0f) minAlt = 64.0f;
    if (minAlt < 42.0f) minAlt = 42.0f;
    float tamAlt = 20.0f;
    float alturas[4];
    float totalAlt;
    while (1){
        totalAlt = 3.0f * gap;
        for (int i = 0; i < 4; i++){
            float h = Textoquebrado(r->FonteJogo, p->alternativas[i], 0, 0,
                                   larguraTexto, tamAlt, WHITE, false);
            alturas[i] = fmaxf(minAlt, h + 14.0f);
            totalAlt += alturas[i];
        }
        if (totalAlt <= alturaRegiao || tamAlt <= 13.0f) break;
        tamAlt -= 1.0f;
    }

    float yAlt = topo;
    for (int i = 0; i < 4; i++){
        Rectangle caixa = {colX, yAlt, colW, alturas[i]};
        bool eliminada = partida->alternativasEliminadas[i];
        bool selecionada = (!partida->respondida && i == partida->select && !eliminada);
        bool correta = ('A' + i == p->alternativa_correta);
        bool escolhidaErrada = (partida->respondida && i == partida->escolha && !correta);

        Color fundo = (Color){14, 25, 48, 238};
        Color borda = (Color){70, 91, 124, 190};
        Color destaque = (Color){85, 115, 155, 255};
        Color corTexto = (Color){235, 241, 250, 255};
        Color corBadge = (Color){29, 45, 72, 255};

        if (eliminada){
            fundo = (Color){24, 29, 40, 190};
            borda = (Color){58, 65, 78, 150};
            destaque = (Color){76, 83, 96, 255};
            corTexto = (Color){128, 137, 151, 255};
            corBadge = (Color){43, 49, 61, 255};
        }else if (selecionada){
            fundo = (Color){20, 48, 69, 250};
            borda = (Color){73, 190, 215, 255};
            destaque = (Color){73, 190, 215, 255};
            corBadge = (Color){32, 126, 151, 255};
        }else if (partida->respondida && correta){
            fundo = (Color){14, 61, 48, 248};
            borda = (Color){69, 205, 151, 255};
            destaque = (Color){69, 205, 151, 255};
            corBadge = (Color){25, 121, 83, 255};
        }else if (escolhidaErrada){
            fundo = (Color){72, 28, 38, 248};
            borda = (Color){225, 91, 105, 255};
            destaque = (Color){225, 91, 105, 255};
            corBadge = (Color){145, 46, 61, 255};
        }else if (partida->respondida){
            fundo = (Color){12, 20, 38, 190};
            borda = (Color){48, 61, 83, 150};
            corTexto = (Color){166, 177, 194, 255};
            corBadge = (Color){31, 42, 60, 255};
        }

        // Sombra discreta, cartão escuro e filete colorido de orientação.
        DrawRectangleRounded((Rectangle){caixa.x, caixa.y + 3, caixa.width, caixa.height},
                             0.16f, 12, Fade(BLACK, 0.30f));
        DrawRectangleRounded(caixa, 0.16f, 12, fundo);
        DrawRectangleRoundedLinesEx(caixa, 0.16f, 12, selecionada ? 2.2f : 1.2f, borda);
        DrawRectangleRounded((Rectangle){caixa.x + 7, caixa.y + 8, 3, caixa.height - 16},
                             0.5f, 6, Fade(destaque, eliminada ? 0.45f : 0.95f));

        // Selo A/B/C/D.
        Rectangle badge = {caixa.x + 18, caixa.y + (caixa.height - 28) / 2, 30, 28};
        DrawRectangleRounded(badge, 0.30f, 8, corBadge);
        const char *letra = TextFormat("%c", 'A' + i);
        Vector2 tamLetra = MeasureTextEx(r->FonteJogo, letra, 17, 1);
        DrawTextEx(r->FonteJogo, letra,
                   (Vector2){badge.x + (badge.width - tamLetra.x) / 2,
                             badge.y + (badge.height - tamLetra.y) / 2},
                   17, 1, WHITE);

        const char *textoAlternativa = eliminada ? "Alternativa eliminada" : p->alternativas[i];
        float alturaTexto = Textoquebrado(r->FonteJogo, textoAlternativa, 0, 0,
                                          larguraTexto, tamAlt, corTexto, false);
        float yTexto = caixa.y + (caixa.height - alturaTexto) / 2.0f;
        Textoquebrado(r->FonteJogo, textoAlternativa, caixa.x + 60, yTexto,
                      larguraTexto, tamAlt, corTexto, true);

        if (selecionada){
            DrawCircleV((Vector2){caixa.x + caixa.width - 17, caixa.y + caixa.height / 2},
                        4.5f, (Color){105, 225, 245, 255});
        }
        yAlt += alturas[i] + gap;
    }

    if (partida->respondida){
        char msg[100];
        Color cormsg;
        if ('A' + partida->escolha == p->alternativa_correta){
            snprintf(msg,sizeof(msg), "CERTA RESPOSTA! +%d MOEDA", MOEDAS_POR_ACERTO);
            cormsg = GREEN;
        }else if (VidasBossNaQuestao(partida->questao) == 1){
            snprintf(msg, sizeof(msg), "ERRADO! BOSS ENFURECIDO: -2 VIDAS");
            cormsg = (Color){255, 105, 115, 255};
        }else{
            snprintf(msg, sizeof(msg), "RESPOSTA ERRADA: a correta era %c", p->alternativa_correta);
            cormsg = RED;
        }
        TextoCentro(r->FonteJogo, msg, 388, 18, cormsg);

        DrawTextEx(r->FonteJogo, "ENTER para continuar | ESC para voltar ao menu", (Vector2){20, 420}, 14, 1, LIGHTGRAY);
        
    }else{
        // Única indicação de ajuda e moedas: discreta, no rodapé.
        DrawTextEx(r->FonteJogo,
                   TextFormat("CIMA/BAIXO escolher   ENTER responder   H AJUDA (5 moedas)   Moedas: %d   ESC menu", partida->moedas),
                   (Vector2){20, 420}, 12, 1, LIGHTGRAY);
    }

    if (partida->dicaMsg[0] != '\0' && !partida->respondida){
        Vector2 tm = MeasureTextEx(r->FonteJogo, partida->dicaMsg, 12, 1);
        DrawTextEx(r->FonteJogo, partida->dicaMsg,
                   (Vector2){(SCREEN_WIDTH - tm.x) / 2.0f, 393}, 12, 1,
                   partida->dicaUsada ? (Color){115, 225, 180, 255} : (Color){255, 205, 125, 255});
    }

    //Roda da Sorte ou fim de jogo por cima de tudo
    if (partida->rodaTipo != RODA_NENHUMA){
        DesenhoRoda(r, partida);
    }else if (partida->fim != FIM_JOGANDO){
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.8f));
        if (partida->fim == FIM_VENCEU){
            TextoCentro(r->FonteJogo, "VOCÊ VENCEU O SHOW!", 150, 40, GOLD);
            TextoCentro(r->FonteJogo, "Todos os bosses foram derrotados. Parabéns!", 210, 20, WHITE);
        }else{
            TextoCentro(r->FonteJogo, "FIM DE JOGO", 150, 40, RED);
            TextoCentro(r->FonteJogo, "Suas vidas acabaram. Tente de novo!", 210, 20, WHITE);
        }
        TextoCentro(r->FonteJogo, "ENTER para voltar ao menu", 290, 18, LIGHTGRAY);
    }
    
}

void Desenhoopcoes(const Recursos *r, const Settings *op){   
    ClearBackground(DARKGRAY);
    
    //fundo escuro para ficar legivel
    DrawTexturePro(r->FundoMenu, 
    (Rectangle){0, 0, (float)r->FundoMenu.width, (float)r->FundoMenu.height},
    (Rectangle){0, 0, SCREEN_WIDTH, SCREEN_HEIGHT},
    (Vector2){0, 0}, 0.0f, WHITE);
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.6f));

    TextoCentro(r->FonteJogo, "OPÇÕES", 40, 38, WHITE);

    const char *itens[TOTAL_OPCOES_ITENS] = {"Volume da música", "Gerenciar Perguntas", "Voltar"};
    for (int i = 0; i < TOTAL_OPCOES_ITENS; i++){
        Rectangle caixa = {100, 130 + i * 70, 600, 56};
        bool sel = (i == op->selecionado);

        Color fundo = sel ? (Color){255, 200, 40, 255} : (Color){15, 25, 110, 255};
        Color borda = sel ? WHITE : (Color){200, 200, 255, 255};
        Color texto = sel ? WHITE : BLACK;

        DrawRectangleRec(caixa, fundo);
        DrawRectangleLinesEx(caixa, 2, borda);

        //Texto do item, centralizado na vertical dentro da caixa
        Vector2 tam = MeasureTextEx(r->FonteJogo, itens[i], 22, 1);
        DrawTextEx(r->FonteJogo, itens[i], (Vector2){caixa.x + 20, caixa.y + (caixa.height - tam.y) / 2}, 22, 1, texto);

        //Barra de volume, só no primeiro item
        if(i == 0){
            Rectangle barra = {caixa.x + 340, caixa.y + 21, 180, 14};
            Rectangle cheio = {barra.x, barra.y, barra.width * op->volume / 100.0f, barra.height};

            DrawRectangleRec(barra, (Color){0, 0, 0, 120});
            DrawRectangleRec(cheio, sel ? (Color){15, 25, 110, 255} : (Color){255, 200, 40, 255});
            DrawRectangleLinesEx(barra, 2, texto);

            char pct[10];
            snprintf(pct, sizeof(pct), "%d%%", op->volume);
            DrawTextEx(r->FonteJogo, pct, (Vector2){barra.x + barra.width + 12, caixa.y + (caixa.height - 18) / 2}, 18, 1, texto);
        }
    }
    
    TextoCentro(r->FonteJogo, "Setas UP/DOWN para escolher | Setas ESQUERDA/DIREITA para ajustar o volume | ESC para voltar ao menu", 420, 14, LIGHTGRAY);
}

int Contarnivel(const Quiz *perguntas, int total, int nivel){
    int cont = 0;
    for (int i = 0; i < total; i++){
        if (perguntas[i].dificuldade == nivel){
            cont++;
        }
        
    }
    return cont;
}

int IndiceporNivel(const Quiz *perguntas, int total, int nivel, int pos){
    int cont = 0;
    for (int i = 0; i < total; i++){
        if (perguntas[i].dificuldade == nivel){
            if (cont == pos){
                return i;   
            }
            cont++;
        }
        
    }
    return -1;
}

//Posição de uma pergunta dentro do seu próprio nível (é o inverso do IndiceporNivel)
int PosnoNivel(const Quiz *perguntas, int total, int idx){
    int pos = 0;
    for (int i = 0; i < idx && i < total; i++){
        if (perguntas[i].dificuldade == perguntas[idx].dificuldade){
            pos++;
        }
    }
    return pos;
}

//Adiciona uma pergunta no final do vetor, se der certo retorna 1
int AddPergunta(Quiz **pergunta, int *total, Quiz nova){
    Quiz *tmp = (Quiz *)realloc(*pergunta, (*total + 1) * sizeof(Quiz));
    if (tmp == NULL){
        exit(EXIT_FAILURE);
    }
    *pergunta = tmp;
    (*pergunta)[*total] = nova;
    (*total)++;
    return 1;
}

//Remove a pergunta de indice idx, deslocando as outras
void RemoverPergunta(Quiz **pergunta, int *total, int idx){
    for (int i = idx; i < *total - 1; i++){
        (*pergunta)[i] = (*pergunta)[i + 1];
    }
    (*total)--;

    if (*total == 0){
        free(*pergunta);
        *pergunta = NULL;
    }else{
        Quiz *tmp = (Quiz *)realloc(*pergunta, (*total) * sizeof(Quiz));
        if (tmp != NULL){
            *pergunta = tmp;
        }
    }
}

//Digitação de texto pelo teclado do raylib (UTF-8, virgula vira ";" por causa do CSV)
void DigitarPergunta(char *destino, int TAMMAX){
    int c;
    while ((c = GetCharPressed()) > 0){
        if (c < 32){
            continue;
        }
        if (c == ','){
            c = ';';
        }
        
        int bytes = 0;
        const char *u = CodepointToUTF8(c, &bytes);
        int len = (int)strlen(destino);
        if (len + bytes < TAMMAX){
            memcpy(destino + len, u, bytes);
            destino[len + bytes] = '\0';
        }
    }

    if ((IsKeyPressed(KEY_SPACE) || (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE))) && destino[0] != '\0'){
        int len = (int)strlen(destino) - 1;
        
        //Não corta uma caracter acentuado no meio
        while (len > 0 && (((unsigned char)destino[len])& 0xC0) == 0x80){
            len--;
        }
        destino[len] = '\0';
    }
} 

//Valida o rascunho e grava. Se algo estiver errado, preenche o g->aviso e continua editando
void SalvarRascunho(GerQuestoes *g, Quiz **perguntas, int *total, FILE **arq){
    Quiz *q = &g->rascunho;

    if (q->enunciado[0] == '\0'){
        snprintf(g->aviso, sizeof(g->aviso), "O enunciado não pode ficar vazio");
        g->campo = 0;
        return;
    }
    
    for (int i = 0; i < 4; i++){
        if (q->alternativas[i][0] == '\0'){
            snprintf(g->aviso, sizeof(g->aviso), "A alternativa %c não pode ficar vazia", 'A' + i);
            g->campo = i + 1;
            return;
        }
    }

    int idx = g->idx;
    if (idx < 0){
        if (!AddPergunta(perguntas, total, *q)){
            snprintf(g->aviso, sizeof(g->aviso), "Erro ao alocar memória");
            return;
        }
        idx = *total - 1;
    }else{
        (*perguntas)[idx] = *q;
    }
    
    F_Salvar_Binario(arq, *perguntas, *total);

    //Mostra a pergunta salva (pode ter mudado de nivel)
    g->nivel = q->dificuldade;
    g->selecionado = PosnoNivel(*perguntas, *total, idx);
    g->topo = 0;
    g->aviso[0] = '\0';
    g->modo = MODO_LISTA;   
}

void atualEdicao(GerQuestoes *g, Quiz **perguntas, int *total, FILE **arq){
    Quiz *q = &g->rascunho;

    if (IsKeyPressed(KEY_ESCAPE)){
        g->modo = MODO_LISTA;
        return;
    }
    
    //troca de campo
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_TAB)){
        g->campo = (g->campo + 1) % TOTAL_CAMPOS;
        g->aviso[0] = '\0';
    }
    if (IsKeyPressed(KEY_UP)){
        g->campo = (g->campo + TOTAL_CAMPOS - 1) % TOTAL_CAMPOS;
        g->aviso[0] = '\0';
    }
    
    if (g->campo == 0){
        DigitarPergunta(q->enunciado, (int)sizeof(q->enunciado));
    }else if (g->campo <= 4){
        DigitarPergunta(q->alternativas[g->campo - 1], (int)sizeof(q->alternativas[0]));
    }else if (g->campo == 5){
        if (IsKeyPressed(KEY_RIGHT)){
            q->alternativa_correta++;
            if (q->alternativa_correta > 'D'){
                q->alternativa_correta = 'A';
            }
        }
        if (IsKeyPressed(KEY_LEFT)){
            q->alternativa_correta--;
            if (q->alternativa_correta < 'A'){
                q->alternativa_correta = 'D';
            }
        }

    }else{
        if (IsKeyPressed(KEY_RIGHT)){
            q->dificuldade++;
            if (q->dificuldade > 5){
                q->dificuldade = 1;
            }
        }
        if (IsKeyPressed(KEY_LEFT)){
            q->dificuldade--;
            if (q->dificuldade < 1){
                q->dificuldade = 5;
            }
        }
    }
    
    //ENTER avança de campo; no último campo, salva
    if (IsKeyPressed(KEY_ENTER)){
        if (g->campo < TOTAL_CAMPOS - 1){
            g->campo++;
            g->aviso[0] = '\0';
        }else{
            SalvarRascunho(g, perguntas, total, arq);
        }
    }
    if (IsKeyPressed(KEY_F5)){
        SalvarRascunho(g, perguntas, total, arq);
    }    
}

void atualRemocao(GerQuestoes *g, Quiz **perguntas, int *total, FILE **arq){
    if (IsKeyPressed(KEY_R) || IsKeyPressed(KEY_ENTER)){
        int idx = IndiceporNivel(*perguntas, *total, g->nivel, g->selecionado);
        if (idx >= 0){
            RemoverPergunta(perguntas, total, idx);
            F_Salvar_Binario(arq, *perguntas, *total);
        }
        g->modo = MODO_LISTA;
    }else if (IsKeyPressed(KEY_N) || IsKeyPressed(KEY_ESCAPE)){
        g->modo = MODO_LISTA;
    }
}

void atualPreguntas(Tela *telaAtual, GerQuestoes *g, Quiz **perguntas, int *total, FILE **arq){
    //Edição e Exclusão
    if (g->modo == MODO_EDITAR){
        atualEdicao(g, perguntas, total, arq);
        return;
    }
    if (g->modo == MODO_REMOVER){
        atualRemocao(g, perguntas, total, arq);
        return;
    }
    
    //Troca de nivel: volta a seleção e a rolagem para o inicio
    if (IsKeyPressed(KEY_RIGHT)){
        g->nivel++;
        if (g->nivel > 5){
            g->nivel = 1;
        }
        g->selecionado = 0;
        g->topo = 0;
    }
    if (IsKeyPressed(KEY_LEFT)){
        g->nivel--;
        if (g->nivel < 1){
            g->nivel = 5;
        }
        g->selecionado = 0;
        g->topo = 0;
    }
    
    int qtd = Contarnivel(*perguntas, *total, g->nivel);

    //Mantém selecionado no limite (importante quando a lista encolher)
    if (g->selecionado >= qtd){
        g->selecionado = (qtd > 0) ? qtd - 1 : 0;
    }
    
    if (qtd > 0){
        if (IsKeyPressed(KEY_DOWN)){
            g->selecionado++;
            if (g->selecionado >= qtd){
                g->selecionado = 0;
            }
        }
        if (IsKeyPressed(KEY_UP)){
            g->selecionado--;
            if (g->selecionado < 0){
                g->selecionado = qtd - 1;
            }
        }
    }

    //Rolagem: garante que o selecionado fique dentro da janela visivel
    if (g->selecionado < g->topo){
        g->topo = g->selecionado;
    }
    if (g->selecionado >= g->topo + LINHAS_VISIVEIS){
        g->topo = g->selecionado - LINHAS_VISIVEIS + 1;
    }
    if (g->topo < 0){
        g->topo = 0;
    }
    
    if (IsKeyPressed(KEY_ESCAPE)){
        *telaAtual = TELA_MENU;
    }

    //N: nova pergunta (já no nivel que está sendo exibido)
    if (IsKeyPressed(KEY_N)){
        g->rascunho = (Quiz){0};
        g->rascunho.dificuldade = g->nivel;
        g->rascunho.alternativa_correta = 'A';
        g->idx = -1;
        g->campo = 0;
        g->aviso[0] = '\0';
        g->modo = MODO_EDITAR;
        while (GetCharPressed() > 0){
            {}; //Descarta o 'n' que acabou de ser apertado
        }
        return;
    }
    
    //ENTER ou E: editar a pergunta selecionada
    if (qtd > 0 && (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_E))){
        int idx = IndiceporNivel(*perguntas, *total, g->nivel, g->selecionado);
        if (idx >= 0){
            g->rascunho = (*perguntas)[idx];
            g->idx = idx;
            g->campo = 0;
            g->aviso[0] = '\0';
            g->modo = MODO_EDITAR;
            while (GetCharPressed() > 0){
                {}; //Descarto o 'e' que acabou de ser apertado
            }
            return;
        }
    }
    
    //DELETE: pede comfirmação para remover
    if (qtd > 0 && (IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_R))){
        g->modo = MODO_REMOVER;
        return;
    }

    if (IsKeyPressed(KEY_ESCAPE)){
        *telaAtual = TELA_MENU;
    }
}

//Corta o texto para caber na largura, acrescentando "..." (respeita UTF-8)
void CortarTexto(Font fonte, char *origem, char *destino, int tamDestino, float tam, float WIDTHMAX){
    snprintf(destino, tamDestino, "%s", origem);
    if (MeasureTextEx(fonte, destino, tam, 1).x <= WIDTHMAX){
        return;
    }
    
    int len = (int)strlen(destino);
    while (len > 0){
        len--;

        //Não corta no meio de um caracter acentuado
        while (len > 0 && (((unsigned char)destino[len])& 0xC0) == 0x80){
            len--;
        }
        destino[len] = '\0';

        char teste[320];
        snprintf(teste, sizeof(teste), "%s...", destino);
        if (MeasureTextEx(fonte, teste, tam, 1).x <= WIDTHMAX){
            snprintf(destino, tamDestino, "%s", teste);
            return;
        }
    }
}

//Mostra o final do texto quando ele não cabe (para o cursor ficar sempre visivel)
void mostrarFinal(Font fonte, const char *origem, char *destino, int TamDestino, float tam, float WidthMAX){
    const char *p = origem;
    while (*p){
        snprintf(destino, TamDestino, "%s", p);
        if (MeasureTextEx(fonte, destino, tam, 1).x <= WidthMAX){
            return;
        }
        p++;
        while ((((unsigned char)*p)& 0xC0) == 0x80){
            p++;
        }
    }
    destino[0] = '\0';
}

void DesenhoPerguntas(const Recursos *r, const Quiz *perguntas, int total, const GerQuestoes *g){
    ClearBackground(DARKGRAY);

    DrawTexturePro(r->FundoMenu, 
    (Rectangle){0, 0, (float)r->FundoMenu.width, (float)r->FundoMenu.height},
    (Rectangle){0, 0, SCREEN_WIDTH, SCREEN_HEIGHT},
    (Vector2){0, 0}, 0.0f, WHITE);
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.6f));
    
    TextoCentro(r->FonteJogo, "GERENCIAR PERGUNTAS", 10, 30, WHITE);

    //Abas do niveis
    const char *nomes[5] = {"Muito Fácil", "Fácil", "Médio", "Difícil", "Muito Difícil"};
    for (int n = 1; n <= 5; n++){
        Rectangle aba = {40 + (n - 1) * 146, 55, 136, 44};
        bool ativa = (n == g->nivel);

        Color fundo = ativa ? (Color){255, 200, 40, 255} : (Color){15, 25, 110, 255};
        Color borda = ativa ? WHITE : (Color){200, 200, 255, 255};
        Color texto = ativa ? BLACK : WHITE;

        DrawRectangleRec(aba, fundo);
        DrawRectangleLinesEx(aba, 2, borda);

        char rotulo[40];
        snprintf(rotulo, sizeof(rotulo), "%d - %s", n, nomes[n - 1]);
        Vector2 t1 = MeasureTextEx(r->FonteJogo, rotulo, 14, 1);
        DrawTextEx(r->FonteJogo, rotulo, (Vector2){aba.x + (aba.width - t1.x) / 2, aba.y + 5}, 14, 1, texto);
    }

    int qtd = Contarnivel(perguntas, total, g->nivel);
    if (qtd == 0){
        TextoCentro(r->FonteJogo, "Nenhuma pergunta neste nível", 220, 22, LIGHTGRAY);
    }
    
    //Só desenha as linhas da janela visivel
    for (int i = 0; i < LINHAS_VISIVEIS; i++){
        int pos = g->topo + i;
        if (pos >= qtd){
            break;
        }
        int idx = IndiceporNivel(perguntas, total, g->nivel, pos);

        Rectangle caixa = {40, 110 + i * LINHAS_HEIGHT, 720, 40};
        bool sel = (pos == g->selecionado);

        Color fundo = sel ? (Color){255, 200, 40, 255} : (Color){15, 25, 110, 255};
        Color borda = sel ? WHITE : (Color){200, 200, 255, 255};
        Color texto = sel ? BLACK : WHITE;

        DrawRectangleRec(caixa, fundo);
        DrawRectangleLinesEx(caixa, 2, borda);

        char linha[400];
        char cortado[320];

        snprintf(linha, sizeof(linha), "%d) %s", pos + 1, perguntas[idx].enunciado);
        CortarTexto(r->FonteJogo, linha, cortado, sizeof(cortado), 18, caixa.width - 30);

        Vector2 tam = MeasureTextEx(r->FonteJogo, cortado, 18, 1);
        DrawTextEx(r->FonteJogo, cortado, (Vector2){caixa.x + 15, caixa.y + (caixa.height - tam.y) / 2}, 18, 1, texto);
    }
    
    //Indicadores de que existem mais perguntas acima ou abaixo
    if (g->topo > 0){
        DrawTextEx(r->FonteJogo, "^", (Vector2){770, 115}, 22, 1, YELLOW);
    }
    if (g->topo + LINHAS_VISIVEIS < qtd){
        DrawTextEx(r->FonteJogo, "v", (Vector2){770, 110 + (LINHAS_VISIVEIS - 1) * LINHAS_HEIGHT}, 22, 1, YELLOW);
    }
    
    char contador[80];
    snprintf(contador, sizeof(contador), "Total geral: %d perguntas", total);
    TextoCentro(r->FonteJogo, contador, 388, 16, YELLOW);
    TextoCentro(r->FonteJogo, "UP/DOWN navegar | LEFT/RIGHT nível | N nova pergunta | E editar | R ou DEL remover | ESC menu", 422, 14, LIGHTGRAY);

    //Confirmação de remoção por cima da lista
    if (g->modo == MODO_REMOVER){
        int idx = IndiceporNivel(perguntas, total, g->nivel, g->selecionado);

        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.7f));
        Rectangle caixa = {120, 130, 560, 170};
        DrawRectangleRec(caixa, (Color){15, 25, 110, 255});
        DrawRectangleLinesEx(caixa, 2, WHITE);

        TextoCentro(r->FonteJogo, "Deseja excluir esta pergunta?", 145, 24, YELLOW);

        if (idx >= 0){
            char copia[400];
            char cortado[320];
            snprintf(copia, sizeof(copia), "%s", perguntas[idx].enunciado);
            CortarTexto(r->FonteJogo, copia, cortado, sizeof(cortado), 16, caixa.width - 40);
            TextoCentro(r->FonteJogo, cortado, 190, 16, WHITE);
        }
        if (qtd <= qtdNecessaria[g->nivel]){
            char aviso[120];
            snprintf(aviso, sizeof(aviso), "Atenção: o nível ficará abaixo do mínimo (%d) e o jogo não vai iniciar.", qtdNecessaria[g->nivel]);
            TextoCentro(r->FonteJogo, aviso, 225, 14, ORANGE);
        }
        
        TextoCentro(r->FonteJogo, "R/DEL = Remover | N/ESC = Cancelar", 256, 16, LIGHTGRAY);
    }
    
}

//Caixa de um campo da edição. Retorna a cor que o texto deve ter
Color DesenhoCaixaCampo(Rectangle caixa, bool ativo){
    DrawRectangleRec(caixa, ativo ? (Color){255, 200, 40, 255} : (Color){15, 25, 110, 255});
    DrawRectangleLinesEx(caixa, 2, ativo ? WHITE : (Color){200, 200, 255, 255});
    return ativo ? BLACK : WHITE;
}

void DesenhoEdicao(const Recursos *r, const GerQuestoes *g){
    ClearBackground(DARKGRAY);

    DrawTexturePro(r->FundoMenu,
    (Rectangle){0, 0, (float)r->FundoMenu.width, (float)r->FundoMenu.height},
    (Rectangle){0, 0, SCREEN_WIDTH, SCREEN_HEIGHT}, (Vector2){0, 0}, 0.0f, WHITE);

    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.6f));

    TextoCentro(r->FonteJogo, (g->idx < 0) ? "NOVA PERGUNTA" : "EDITAR PERGUNTA", 10, 30, WHITE);

    const Quiz *q = &g->rascunho;
    bool piscar = (((int)(GetTime() * 2)) & 2) == 0;//cursor piscando

    //Campo 0: enunciado(pode quebrar varias linhas)
    bool ativo = (g->campo == 0);
    Rectangle caixa = {40, 55, 720, 80};
    Color cor = DesenhoCaixaCampo(caixa, ativo);
    if(q->enunciado[0] == '\0' && !ativo){
        DrawTextEx(r->FonteJogo, "Enunciado", (Vector2){caixa.x + 12, caixa.y + 8}, 16, 1, LIGHTGRAY);
    }else{
        char buf[1100];
        snprintf(buf, sizeof(buf), "%s%s", q->enunciado, (ativo && piscar) ? "_" : "");
        Textoquebrado(r->FonteJogo, buf, caixa.x + 12, caixa.y + 8, caixa.width - 24, 16, cor, true);
    }

    //Campos 1 a 4: alternativas A a D
    for (int i = 0; i < 4; i++){
        ativo = (g->campo == i + 1);
        caixa = (Rectangle){40, 145 + i * 40, 720, 34};
        cor = DesenhoCaixaCampo(caixa, ativo);

        char linha[600];
        char visivel[600];
        snprintf(linha, sizeof(linha), "%c) %s%s", 'A' + i, q->alternativas[i], (ativo && piscar) ? "_" : "");
        mostrarFinal(r->FonteJogo, linha, visivel, sizeof(visivel), 18, caixa.width - 24);
        DrawTextEx(r->FonteJogo, visivel, (Vector2){caixa.x + 12, caixa.y + (caixa.height - 18) / 2}, 18, 1, cor);
    }
    
    //Campo 5: alternativa correta
    ativo = (g->campo == 5);
    caixa = (Rectangle){40, 310, 350, 34};
    cor = DesenhoCaixaCampo(caixa, ativo);
    char texto[120];
    snprintf(texto, sizeof(texto), "Alternativa correta: < %c >", q->alternativa_correta);
    DrawTextEx(r->FonteJogo, texto, (Vector2){caixa.x + 12, caixa.y + (caixa.height - 18) / 2}, 18, 1, cor);
    
    //Campo 6: dificuldade
    ativo = (g->campo == 6);
    caixa = (Rectangle){410, 310, 350, 34};
    cor = DesenhoCaixaCampo(caixa, ativo);
    snprintf(texto, sizeof(texto), "Dificuldade: < %s >", F_Nome_Dificuldade(q->dificuldade));
    DrawTextEx(r->FonteJogo, texto, (Vector2){caixa.x + 12, caixa.y + (caixa.height - 18) / 2}, 18, 1, cor);

    if (g->aviso[0] != '\0'){
        TextoCentro(r->FonteJogo, g->aviso, 358, 16, ORANGE);
    }
    
    TextoCentro(r->FonteJogo, "Setas UP/DOWN ou ENTER para trocar de campo | Setas LEFT/RIGHT para ajustar | Tecla F5 para salvar | ESC para cancelar", 422, 14,LIGHTGRAY);
}

//main
int main(){
    
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1920, 1080, "Jogo do Milhão"); // Inicializa a janela
    SetWindowMinSize(SCREEN_WIDTH, SCREEN_HEIGHT);
    SetTargetFPS(60); // Define a taxa de quadros
    SetExitKey(KEY_NULL);//o jogo não fecha com a tecla ESC
    InitAudioDevice();

    //Recursos, Layout e estado inicial:
    Recursos recursos = LoadRecursos();
    Layout layout = CalcularLayout(&recursos);
    Menu menu = criarMenu();

    FILE *arqPerguntas = NULL;
    int totalPerguntas = 0;
    Quiz *perguntas = F_Inicializa_Sistema(&arqPerguntas, &totalPerguntas);

    Partida partida = {0};
    Settings opcoes = {0, 80};
    GerQuestoes gerPerg = {.selecionado = 0, .topo = 0, .nivel = 1};
    srand((unsigned int)time(NULL));

    Tela telaAtual = TELA_MENU;
    bool sair = false;

    // Loop principal
    while (!WindowShouldClose() && !sair){
        // Acoes e desenhos acontecem aqui dentro
        
        //Música que toca no menu principal
        UpdateMusicStream(recursos.musicaMenu);
        
        Tela telaAnterior = telaAtual;
        switch (telaAtual){
        case TELA_MENU:
            atualMenu(&menu, &telaAtual, &sair, &recursos);
            break;
        case TELA_INTRO:
            atualIntro(&telaAtual, &recursos);
            break;
        case TELA_JOGO:
            atualJogo(&telaAtual, &recursos, &partida, perguntas, totalPerguntas);
            break;
        case TELA_OPCOES:
            atualOpcoes(&telaAtual, &recursos, &opcoes);
            break;
        case TELA_PERGUNTAS:
            atualPreguntas(&telaAtual, &gerPerg, &perguntas, &totalPerguntas, &arqPerguntas);
            break;
        }

        if (telaAnterior == TELA_INTRO && telaAtual == TELA_JOGO){
            TraceLog(LOG_INFO, "mudou da introdução para o jogo");
            IniciarPartida(&partida, perguntas, totalPerguntas);
            TraceLog(LOG_INFO, "podeJogar=%d questao=%d sorteada=%d", partida.podeJogar, partida.questao, partida.sorteada);
        }
        
        
        //F11 alterna tela cheia
        if (IsKeyPressed(KEY_F11)){
            ToggleFullscreen();
        }

        //Amplia a tela lógica (800x450) para o tamanho real da janela, mantendo a proporção
        float escala = fminf((float)GetScreenWidth() / SCREEN_WIDTH, (float)GetScreenHeight() / SCREEN_HEIGHT);
        Camera2D camera = {0};
        camera.zoom = escala;
        camera.offset = (Vector2){(GetScreenWidth() - SCREEN_WIDTH * escala) / 2, (GetScreenHeight() - SCREEN_HEIGHT * escala) / 2};

        BeginDrawing();
            ClearBackground(BLACK);
            BeginMode2D(camera);
            switch (telaAtual){
            case TELA_MENU:
                DesenhoMenu(&menu, &recursos, &layout);
                break;
            case TELA_INTRO:
                DesenhoIntro(&recursos);
                break;
            case TELA_JOGO:
                DesenhoJogo(&recursos, perguntas ,totalPerguntas, &partida);
                break;
            case TELA_OPCOES:
                Desenhoopcoes(&recursos, &opcoes);
                break;
            case TELA_PERGUNTAS:
                if (gerPerg.modo == MODO_EDITAR){
                    DesenhoEdicao(&recursos, &gerPerg);
                }else{
                    DesenhoPerguntas(&recursos, perguntas, totalPerguntas, &gerPerg);
                }
                break;
            }
            EndMode2D();
        EndDrawing();
    }

    //Limpeza de dados
    free(partida.usadas);
    free(perguntas);
    UnloadRecursos(&recursos);
    // Finalizacao
    CloseAudioDevice();
    CloseWindow(); // Fecha a janela
    return 0;
}