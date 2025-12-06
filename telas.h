#ifndef TELAS_H
#define TELAS_H

#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>

// Janela
ALLEGRO_DISPLAY* criar_tela(int largura, int altura, ALLEGRO_COLOR cor);
void destruir_display(ALLEGRO_DISPLAY* display);

// Desenho
void atualizar_tela(void (*desenhar)(void));

// Utilitários
bool tela_rodando();
bool mouse_esta_clicado();
int mouse_pos_x();
int mouse_pos_y();

// Largura/altura da janela
int pegar_largura_tela(void);
int pegar_altura_tela(void);

// Callbacks/Init/Destroy para telas usadas pela app
void desenhar_formas_menu(void);
void desenhar_tela_invest(void);
void tela_invest_init(void);
void tela_invest_destroy(void);
void tela_invest_new(void); // start new game

#endif // TELAS_H