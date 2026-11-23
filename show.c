#include "raylib.h"
#include <math.h>
#include "quiz.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 450

#define TOTAL_QUESTOES 15

const int NiveisporQuestao[TOTAL_QUESTOES] = {1, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5};
const int qtdNecessaria[6] = {0, 2, 2, 4, 4, 3};

#define TOTAL_OPCOES 3
#define TOTAL_OPCOES_ITENS 3 //Volume da música, Gerenciamento de perguntas e voltar
#define PAINEL_WIDTH 300

#define LINHAS_VISIVEIS 6
#define LINHAS_HEIGHT 44

#define TOTAL_CAMPOS 7 //0 Enunciado | 1 - 4 Alternativas | 5 Correta | 6 Dificuldade

typedef enum{TELA_MENU, TELA_JOGO, TELA_OPCOES, TELA_PERGUNTAS} Tela;

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
}Recursos;

//Estado da partida em andamento
typedef struct {
    int questao;
    int sorteada;
    int select;
    bool respondida;
    int escolha;
    int *usadas;
    bool podeJogar;
    char aviso[160];
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

const char *MenuOP[TOTAL_OPCOES] = {"Jogar", "Opcoes", "Sair"};

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

    //Fonte e logo
    r.FonteMenu = LoadFont("recursos/ArialCEBoldItalic.ttf");
    r.logo = LoadTexture("recursos/logo.png");
    SetTextureFilter(r.logo, TEXTURE_FILTER_BILINEAR);

    //Fundos: Menu e Jogo
    r.FundoMenu = LoadTexture("recursos/FundoJogo.png");
    SetTextureFilter(r.FundoMenu,TEXTURE_FILTER_BILINEAR);
    r.FundoJogo = LoadTexture("recursos/FundoJogo.png");
    SetTextureFilter(r.FundoJogo, TEXTURE_FILTER_BILINEAR);
    
    //Fonte do Jogo: Latin-1 - acentos + aspas curvas, travessões e reticências
    int codepoints[224 + 7];
    int n = 0;

    //32 a 255: alfabeto, letras acentuadas, números e pontuação
    for(int c = 32; c <= 255; c++){
        codepoints[n++] = c;
    }

    //Extras: - — ‘ ’ “ ” …
    const int extras[] = {0x2013, 0x2014, 0x2018, 0x2019, 0x201C, 0x201D, 0x2026};
    for (int i = 0; i < 7; i++){
        codepoints[n++] = extras[i];
    }
    r.FonteJogo = LoadFontEx("recursos/ArialCEBoldItalic.ttf", 32, codepoints, n);
    SetTextureFilter(r.FonteJogo.texture, TEXTURE_FILTER_BILINEAR);

    return r;
}

//Função do Unload de recursos
void UnloadRecursos(Recursos *r){
    UnloadTexture(r->logo);
    UnloadTexture(r->FundoMenu);
    UnloadTexture(r->FundoJogo);
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
            *telaAtual = TELA_JOGO;
            PlaySound(r->Somselecao);
            PauseMusicStream(r->musicaMenu);
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

void sortearPergunta(Partida *partida, const Quiz *perguntas, int total){
    int nivel = NiveisporQuestao[partida->questao];
    int sorteada = -1;

    while (sorteada == -1){
        int indice = rand() % total;
        if (perguntas[indice].dificuldade == nivel && partida->usadas[indice] == 0){
            sorteada = indice;
            partida->usadas[indice] = 1;
        }
        
    }
    
    partida->sorteada = sorteada;
}

void IniciarPartida(Partida *partida, const Quiz *perguntas, int total){
    free(partida->usadas);
    *partida = (Partida){0};
    partida->podeJogar = true;

    for (int niv = 1; niv <= 5; niv++){
        int cont = 0;
        for (int i = 0; i < total; i++){
            if (perguntas[i].dificuldade == niv){
                cont++;
            }
            
        }
        if (cont < qtdNecessaria[niv]){
                partida->podeJogar = false;
                snprintf(partida->aviso, sizeof(partida->aviso), "O nivel %s precisa ter pelo menos %d perguntas (atual: %d)", F_Nome_Dificuldade(niv), qtdNecessaria[niv], cont);
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

void atualJogo(Tela *telaAtual, const Recursos *r, Partida *partida, const Quiz *perguntas, int total){
    
    if (!partida->podeJogar){
        if (IsKeyPressed(KEY_ENTER)){//Não para jogar: ENTER e volta ao menu
            VoltarAoMenu(telaAtual, r);
        }
        
    }else if (!partida->respondida){
        if (IsKeyPressed(KEY_DOWN)){
            partida->select++;
            if (partida->select > 3){
                partida->select = 0;
            }
        }
        if (IsKeyPressed(KEY_UP)){
            partida->select--;
            if (partida->select < 0){
                partida->select = 3;
            }
        }
        if (IsKeyPressed(KEY_ENTER)){
            partida->escolha = partida->select;
            partida->respondida = true;
            PlaySound(r->Somselecao);
        }

    }else if (IsKeyPressed(KEY_ENTER)){
        //Já respondeu: ENTER avança
        bool Acertou = ('A' + partida->escolha == perguntas[partida->sorteada].alternativa_correta);

        if (!Acertou || partida->questao == TOTAL_QUESTOES - 1){
            VoltarAoMenu(telaAtual, r);
        }else{
            partida->questao++;
            sortearPergunta(partida, perguntas, total);
            partida->select = 0;
            partida->respondida = false;
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

void DesenhoJogo(const Recursos *r, const Quiz *perguntas, int total, const Partida *partida){
    ClearBackground(DARKBLUE);

    DrawTexturePro(r->FundoJogo,
    (Rectangle){0, 0, (float)r->FundoJogo.width, (float)r->FundoJogo.height},
    (Rectangle){0, 0, SCREEN_WIDTH, SCREEN_HEIGHT},
    (Vector2){0, 0}, 0.0f, WHITE);
    
    if (total == 0){
        DrawTextEx(r->FonteJogo, "Nenhuma pergunta carregada!", (Vector2){20, 20}, 24, 1, WHITE);
        return;
    }
    
    const Quiz *p = &perguntas[partida->sorteada]; //Por enquanto sempre a primeira pergunta
    char texto[200];
    
    snprintf(texto, sizeof(texto), "Questao: %d / %d | Dificuldade: %s", partida->questao + 1, TOTAL_QUESTOES,F_Nome_Dificuldade(p->dificuldade));
    DrawTextEx(r->FonteJogo, texto, (Vector2){20, 12}, 18, 1, YELLOW);

    if (partida->questao == TOTAL_QUESTOES - 1){
        Vector2 tamFinal = MeasureTextEx(r->FonteJogo, "PERGUNTA FINAL!", 18, 1);
        DrawTextEx(r->FonteJogo, "PERGUNTA FINAL!", (Vector2){SCREEN_WIDTH - 20 - tamFinal.x, 12}, 18, 1, ORANGE);
    }
    
    //Enunciado
    Textoquebrado(r->FonteJogo, p->enunciado, 20, 58, 760, 20, WHITE, true);
    
    //Alternativas: A a D
    for (int i = 0; i < 4; i++){
        Rectangle caixa = {20, 165 + i * 52, 760, 46};
        
        Color fundo = (Color){15, 25, 110, 255};
        Color borda = (Color){200, 200, 255, 255};
        Color corTexto = WHITE;

        if (!partida->respondida){
            //A caixa selecionada fica dourada
            if (i == partida->select){
                fundo = (Color){255, 200, 40, 255};
                borda = WHITE;
                corTexto = BLACK;
            }
        }else{
            //A certa fica verde e a errada fica vermelha
            if ('A' + i == p->alternativa_correta){
                fundo = (Color){0, 150, 60, 255};
                borda = WHITE;
            }else if (i == partida->escolha){
                fundo = (Color){190, 30, 30, 255};
                borda = WHITE;
            }
        }
        
        DrawRectangleRec(caixa, fundo);
        DrawRectangleLinesEx(caixa, 2, borda);

        snprintf(texto, sizeof(texto), "%c) %s", 'A' + i, p->alternativas[i]);
        float Altura = Textoquebrado(r->FonteJogo, texto, 0, 0, 730, 18, corTexto, false);
        float ty = caixa.y + (caixa.height - Altura)/2;
        Textoquebrado(r->FonteJogo, texto, caixa.x + 15, ty, 730, 18, corTexto, true);
    }

    if (partida->respondida){
        char msg[100];
        Color cormsg;
        if ('A' + partida->escolha == p->alternativa_correta){
            snprintf(msg,sizeof(msg), "CERTA RESPOSTA!");
            cormsg = GREEN;
        }else{
            snprintf(msg, sizeof(msg), "RESPOSTA ERRADA: A correta era %c", p->alternativa_correta);
            cormsg = RED;
        }
        TextoCentro(r->FonteJogo, msg, 380, 22, cormsg);

        DrawTextEx(r->FonteJogo, "ENTER para continuar | ESC para voltar ao menu", (Vector2){20, 420}, 14, 1, LIGHTGRAY);
        
    }else{
        DrawTextEx(r->FonteJogo, "Setas UP/DOWN para escolher | ENTER para confirmar | ESC para voltar ao menu", (Vector2){20, 420}, 14, 1, LIGHTGRAY);
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

    TextoCentro(r->FonteJogo, "OPÇÔES", 40, 38, WHITE);

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
            len++;
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

        TextoCentro(r->FonteJogo, "Quer excluir está pergunta?", 145, 24, YELLOW);

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
    
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Jogo do Milhão"); // Inicializa a janela
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

        if (telaAnterior == TELA_MENU && telaAtual == TELA_JOGO){
            TraceLog(LOG_INFO, "mudou do menu para o jogo");
            IniciarPartida(&partida, perguntas, totalPerguntas);
            TraceLog(LOG_INFO, "podeJogar=%d questao=%d sorteada=%d", partida.podeJogar, partida.questao, partida.sorteada);
        }
        
        
        BeginDrawing();
            switch (telaAtual){
            case TELA_MENU:
                DesenhoMenu(&menu, &recursos, &layout);
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