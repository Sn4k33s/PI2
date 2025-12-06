#include "textos.h"

// Desenha texto na tela usando a fonte e cor fornecidas.
// "flag" controla o alinhamento (ALLEGRO_ALIGN_CENTRE etc.).
void texto(ALLEGRO_FONT* font, const char* texto_str, ALLEGRO_COLOR cor, int x, int y, int flag) {
    if (!font || !texto_str) return;
    al_draw_text(font, cor, x, y, flag, texto_str);
}
