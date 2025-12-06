#ifndef TEXTOS_H
#define TEXTOS_H

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>

// alignment constants fallback (defined by Allegro's font addon normally)
#ifndef ALLEGRO_ALIGN_LEFT
#define ALLEGRO_ALIGN_LEFT 0
#define ALLEGRO_ALIGN_CENTRE 1
#define ALLEGRO_ALIGN_RIGHT 2
#endif

// Desenha texto na tela usando a fonte, cor e posição fornecidas.
// O parâmetro 'str' evita conflito com o nome da função.
void texto(ALLEGRO_FONT* font, const char* str, ALLEGRO_COLOR cor, int x, int y, int flag);

#endif
