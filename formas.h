#ifndef FORMAS_H
#define FORMAS_H

#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>

// Funções simples para desenhar formas. Cada função usa o Allegro para desenhar.
void retangulo_preenchido(float x1, float y1, float x2, float y2, ALLEGRO_COLOR cor);
void retangulo_bordas(float x1, float y1, float x2, float y2, ALLEGRO_COLOR cor, float espessura);
void circulo_preenchido(float x, float y, float raio, ALLEGRO_COLOR cor); 
void circulo_bordas(float x, float y, float raio, ALLEGRO_COLOR cor, float espessura);
void linha(float x1, float y1, float x2, float y2, ALLEGRO_COLOR cor, float espessura);
void triangulo_preenchido(float x1, float y1, float x2, float y2, float x3, float y3, ALLEGRO_COLOR cor);
void triangulo_bordas(float x1, float y1, float x2, float y2, float x3, float y3, ALLEGRO_COLOR cor, float espessura); 
void geometria_menu(void);
void aba_padrao(int x, int y, int x2, int y2);
void barra_de_tarefas();
void tela_menu(int x, int y, int x2, int y2);
void tela_invest(int x, int y, int x2, int y2);

// Font management for investment screen
void tela_invest_fonts_init(void);
void tela_invest_fonts_destroy(void);

// Draw player UI boxes
void draw_player_balance_box(int x, int y, int w, int h);
void draw_player_history_box(int x, int y, int w, int h, int max_lines);

// Handle mouse click in investment screen
void investir_handle_click(int mx, int my);
// Handle mouse move in investment screen (hover)
void investir_handle_mouse_move(int mx, int my);

#endif