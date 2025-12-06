#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>
#include "telas.h"
#include "formas.h"
#include "textos.h"
#include "hover_mouse.h"

// Geometria do menu (nomes mais simples)
int menu_x = 240;
int menu_y = 180;
int menu_x2 = 1040;

// Desenha o menu (callback)
void desenhar_formas_menu(void) {
    // desenha painel do menu
    tela_menu(menu_x, menu_y, menu_x2, 780);
    barra_de_tarefas();
    aba_padrao(10, 924, 100, 955);

    // destaque por hover (variavel definida em hover_mouse.c)
    extern int menu_hover;
    if (menu_hover >= 0) {
        int x = menu_x, y = menu_y, x2 = menu_x2;
        if (menu_hover == 0) retangulo_bordas(x + 8, y + 208, x2 - 8, y + 252, al_map_rgb(255,215,0), 3.0);
        else if (menu_hover == 1) retangulo_bordas(x + 8, y + 268, x2 - 8, y + 312, al_map_rgb(255,215,0), 3.0);
        else if (menu_hover == 2) retangulo_bordas(x + 8, y + 328, x2 - 8, y + 372, al_map_rgb(255,215,0), 3.0);
    }
}

// prototipo da tela de investimento (em telas.h)
void desenhar_tela_invest(void);

int main() {
    // inicializacao do Allegro e addons
    if (!al_init()) return -1;
    al_init_font_addon();
    al_init_ttf_addon();
    al_init_primitives_addon();
    al_install_keyboard();
    al_install_mouse();

    ALLEGRO_DISPLAY* tela = criar_tela(1280, 960, al_map_rgb(0, 128, 128));
    if (!tela) return -1;

    ALLEGRO_EVENT_QUEUE* fila = al_create_event_queue();
    if (!fila) { destruir_display(tela); return -1; }

    al_register_event_source(fila, al_get_display_event_source(tela));
    al_register_event_source(fila, al_get_keyboard_event_source());
    al_register_event_source(fila, al_get_mouse_event_source());

    bool rodando = true;
    int tela_atual = 0; // 0 = menu, 1 = invest

    while (rodando) {
        ALLEGRO_EVENT event;
        while (al_get_next_event(fila, &event)) {
            if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) { rodando = false; }
            else if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
                if (tela_atual == 0) {
                    if (event.keyboard.keycode == ALLEGRO_KEY_ENTER) {
                        // Continuar -> carrega save do disco
                        tela_invest_destroy();
                        tela_invest_init();
                        tela_atual = 1;
                    }
                    else if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) rodando = false;
                    else if (event.keyboard.keycode == ALLEGRO_KEY_A) {
                        // Novo jogo -> gera novo estado
                        tela_invest_new();
                        tela_atual = 1;
                    }
                } else if (tela_atual == 1) {
                    if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) tela_atual = 0;
                }
            }
            else if (event.type == ALLEGRO_EVENT_MOUSE_AXES || event.type == ALLEGRO_EVENT_MOUSE_ENTER_DISPLAY) {
                if (tela_atual == 0) atualizar_hover_menu(event.mouse.x, event.mouse.y);
                else if (tela_atual == 1) investir_handle_mouse_move(event.mouse.x, event.mouse.y);
            }
            else if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
                if (event.mouse.button & 1) {
                    if (tela_atual == 0) {
                        atualizar_hover_menu(event.mouse.x, event.mouse.y);
                        if (menu_hover == 0) { // Novo Jogo
                            tela_invest_new(); tela_atual = 1;
                        } else if (menu_hover == 1) { // Continuar
                            tela_invest_destroy(); tela_invest_init(); tela_atual = 1;
                        } else if (menu_hover == 2) rodando = false;
                    } else if (tela_atual == 1) {
                        // encaminha clique para tela de investimento
                        investir_handle_click(event.mouse.x, event.mouse.y);
                    }
                }
            }
        }

        // If invest screen requested to return to menu, handle it here
        if (tela_atual == 1) {
            if (tela_invest_consume_return_request()) {
                // cleanup invest screen and go back to menu
                tela_invest_destroy();
                tela_atual = 0;
            }
        }

        if (tela_atual == 0) atualizar_tela(desenhar_formas_menu);
        else if (tela_atual == 1) atualizar_tela(desenhar_tela_invest);
    }

    // limpeza final
    tela_invest_destroy();
    al_destroy_event_queue(fila);
    destruir_display(tela);
    return 0;
}
