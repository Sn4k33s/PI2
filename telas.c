#include <allegro5/allegro.h>
#include <allegro5/allegro_opengl.h>
#include <allegro5/allegro_primitives.h>
#include "telas.h"
#include "formas.h"



// Variáveis internas simples
static ALLEGRO_DISPLAY* display_principal = NULL;
static ALLEGRO_COLOR cor_fundo;
static int largura_atual = 1280; // largura da janela
static int altura_atual = 960;   // altura da janela

// Cria a janela do jogo e define a cor de fundo
ALLEGRO_DISPLAY* criar_tela(int largura, int altura, ALLEGRO_COLOR cor) {
    // assume que o programa já chamou al_init() antes
    display_principal = al_create_display(largura, altura);
    cor_fundo = cor;
    largura_atual = largura;
    altura_atual = altura;
    if (display_principal) {
        al_clear_to_color(cor_fundo);
        al_flip_display();
    }
    return display_principal;
}

// Função para pegar a largura atual da janela
int pegar_largura_tela() {
    if (display_principal)
        return al_get_display_width(display_principal);
    return largura_atual;
}

// Função para pegar a altura atual da janela
int pegar_altura_tela() {
    if (display_principal)
        return al_get_display_height(display_principal);
    return altura_atual;
}

// Desenha usando a função passada e atualiza a janela.
// Responsivo: verifica se a janela mudou de tamanho e atualiza variáveis.
void atualizar_tela(void (*desenhar)(void)) {
    if (!display_principal) return;
    // Atualiza largura/altura se a janela foi redimensionada
    largura_atual = al_get_display_width(display_principal);
    altura_atual = al_get_display_height(display_principal);
    // Limpa a tela com a cor de fundo
    al_clear_to_color(cor_fundo);
    // Chama a função de desenho, que pode usar pegar_largura_tela/pegar_altura_tela
    if (desenhar) desenhar();
    // Mostra o que foi desenhado
    al_flip_display();
}

// Destroi a janela e limpa variáveis
void destruir_display(ALLEGRO_DISPLAY* display) {
    if (display) al_destroy_display(display);
    display_principal = NULL;
}

// Funções auxiliares simples mantidas para compatibilidade.
bool tela_rodando() {
    // Se a janela existe, considere que está rodando
    return display_principal != NULL;
}

bool mouse_esta_clicado() {
    // Esta versão simples não rastreia o mouse aqui.
    return false;
}

int mouse_pos_x() { return 0; }
int mouse_pos_y() { return 0; }

