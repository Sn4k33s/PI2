#ifndef HOVER_MOUSE_H
#define HOVER_MOUSE_H

// Atualiza o índice do item em hover com base na posição do mouse.
// mx, my: coordenadas do mouse (pixels)
void atualizar_hover_menu(int mx, int my);

// Variável global que indica qual item está com hover:
// -1 = nenhum, 0..2 = índice da opção (Novo Jogo, Continuar, Sair)
extern int menu_hover;

#endif // HOVER_MOUSE_H