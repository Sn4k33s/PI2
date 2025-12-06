#include "hover_mouse.h"
#include "formas.h"

// Constantes da geometria do menu (mantenha em sincronia com main.c)
#define MENU_X 240
#define MENU_Y 180
#define MENU_X2 1040

// Variável global usada por main.c para saber o hover atual.
// Inicialmente -1 = nenhum
int menu_hover = -1;

// Função auxiliar local: verifica se (mx,my) está dentro do retângulo [x1,y1]-[x2,y2]
static int in_rect(int mx, int my, int x1, int y1, int x2, int y2)
{
    return (mx >= x1 && mx <= x2 && my >= y1 && my <= y2);
}

// Função pública: atualiza menu_hover conforme posição do mouse.
// Foi removido "static" porque main.c chama essa função.
void atualizar_hover_menu(int mx, int my)
{
    int x = MENU_X;
    int y = MENU_Y;
    int x2 = MENU_X2;

    // As coordenadas aqui correspondem às usadas ao desenhar destaque no menu.
    if (in_rect(mx, my, x + 8,  y + 208, x2 - 8,  y + 252)) menu_hover = 0;
    else if (in_rect(mx, my, x + 8,  y + 268, x2 - 8,  y + 312)) menu_hover = 1;
    else if (in_rect(mx, my, x + 8,  y + 328, x2 - 8,  y + 372)) menu_hover = 2;
    else menu_hover = -1;
}