#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "formas.h"
#include "textos.h"
#include "bolsas.h"
#include "player.h"
#include "tela_invest.h"
#include "noticias.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>

// --- Fontes usadas na tela de investimentos (inicializadas uma vez)
static ALLEGRO_FONT* fonte_titulo = NULL;
static ALLEGRO_FONT* fonte_linha = NULL;
static ALLEGRO_FONT* fonte_pequena = NULL;

// --- Estado da UI
static int aba_direita = 0;        // 0 = historico, 1 = portfolio
static int bolsa_selecionada = -1; // indice da bolsa selecionada
static int bolsa_hover = -1;       // indice em hover

static int main_aba = 0; // 0 = investimentos, 1 = mae, 2 = noticias, 3 = tutorial, 4 = dicas


// Inicializa fontes (chamar uma vez)
void tela_invest_fonts_init(void)
{
    if ( ! fonte_titulo ) {
        fonte_titulo = al_load_ttf_font("arial.ttf", 18, 0);
        if ( ! fonte_titulo ) fonte_titulo = al_create_builtin_font();
    }

    if ( ! fonte_linha ) {
        fonte_linha = al_load_ttf_font("arial.ttf", 16, 0);
        if ( ! fonte_linha ) fonte_linha = al_create_builtin_font();
    }

    if ( ! fonte_pequena ) {
        fonte_pequena = al_load_ttf_font("arial.ttf", 14, 0);
        if ( ! fonte_pequena ) fonte_pequena = al_create_builtin_font();
    }
}

// Destroi fontes (quando sair da tela)
void tela_invest_fonts_destroy(void)
{
    if ( fonte_titulo ) { al_destroy_font(fonte_titulo); fonte_titulo = NULL; }
    if ( fonte_linha )   { al_destroy_font(fonte_linha);   fonte_linha   = NULL; }
    if ( fonte_pequena ) { al_destroy_font(fonte_pequena); fonte_pequena = NULL; }
}

// Funcoes de desenho basicos 
void retangulo_preenchido(float x1, float y1, float x2, float y2, ALLEGRO_COLOR cor)
{
    al_draw_filled_rectangle(x1, y1, x2, y2, cor);
}
void retangulo_bordas(float x1, float y1, float x2, float y2, ALLEGRO_COLOR cor, float espessura)
{
    al_draw_rectangle(x1, y1, x2, y2, cor, espessura);
}
void linha(float x1, float y1, float x2, float y2, ALLEGRO_COLOR cor, float espessura)
{
    al_draw_line(x1, y1, x2, y2, cor, espessura);
}

// Helper: draw wrapped text in a rectangle and return the new Y position after drawing.
static int texto_draw_wrapped(ALLEGRO_FONT* font, const char* text, ALLEGRO_COLOR cor, int x, int y, int max_w, int max_h)
{
    if (!font || !text) return y;
    int text_bottom = y + max_h;
    const char* p = text;
    float space_w = al_get_text_width(font, " ");

    while (*p && y <= text_bottom) {
        char para[2048]; int pi = 0;
        while (*p && *p != '\n' && pi < (int)sizeof(para) - 1) para[pi++] = *p++;
        para[pi] = '\0'; if (*p == '\n') ++p;

        const char* wp = para;
        char linebuf[2048]; linebuf[0] = '\0';
        float line_w = 0.0f;

        while (*wp && y <= text_bottom) {
            // skip spaces
            while (*wp == ' ') ++wp;
            if (!*wp) break;
            // get next word
            char word[256]; int wi = 0;
            while (*wp && *wp != ' ' && wi < (int)sizeof(word) - 1) word[wi++] = *wp++;
            word[wi] = '\0';
            float w = al_get_text_width(font, word);

            if (linebuf[0] == '\0') {
                if (w <= (float)max_w) {
                    snprintf(linebuf, sizeof(linebuf), "%s", word);
                    line_w = w;
                } else {
                    // split very long word into parts that fit
                    char part[256]; int pi2 = 0;
                    for (int k = 0; word[k] != '\0' && y <= text_bottom; ++k) {
                        part[pi2++] = word[k]; part[pi2] = '\0';
                        float pw = al_get_text_width(font, part);
                        if (pw > (float)max_w) {
                            part[pi2-1] = '\0';
                            al_draw_text(font, cor, x, y, ALLEGRO_ALIGN_LEFT, part);
                            y += al_get_font_line_height(font);
                            // start new part with current char
                            part[0] = word[k]; part[1] = '\0'; pi2 = 1;
                        }
                    }
                    if (pi2 > 0 && y <= text_bottom) {
                        al_draw_text(font, cor, x, y, ALLEGRO_ALIGN_LEFT, part);
                        y += al_get_font_line_height(font);
                    }
                    linebuf[0] = '\0'; line_w = 0.0f;
                }
            } else {
                if (line_w + space_w + w <= (float)max_w) {
                    int len = (int)strlen(linebuf);
                    snprintf(linebuf + len, sizeof(linebuf) - len, " %s", word);
                    line_w += space_w + w;
                } else {
                    // render current line and start new
                    al_draw_text(font, cor, x, y, ALLEGRO_ALIGN_LEFT, linebuf);
                    y += al_get_font_line_height(font);
                    if (y > text_bottom) break;
                    if (w <= (float)max_w) {
                        snprintf(linebuf, sizeof(linebuf), "%s", word);
                        line_w = w;
                    } else {
                        // long word at line start -> split
                        char part[256]; int pi2 = 0;
                        for (int k = 0; word[k] != '\0' && y <= text_bottom; ++k) {
                            part[pi2++] = word[k]; part[pi2] = '\0';
                            float pw = al_get_text_width(font, part);
                            if (pw > (float)max_w) {
                                part[pi2-1] = '\0';
                                al_draw_text(font, cor, x, y, ALLEGRO_ALIGN_LEFT, part);
                                y += al_get_font_line_height(font);
                                part[0] = word[k]; part[1] = '\0'; pi2 = 1;
                            }
                        }
                        if (pi2 > 0 && y <= text_bottom) {
                            snprintf(linebuf, sizeof(linebuf), "%s", part);
                            line_w = al_get_text_width(font, linebuf);
                        } else { linebuf[0] = '\0'; line_w = 0.0f; }
                    }
                }
            }
        }

        if (linebuf[0] != '\0' && y <= text_bottom) {
            al_draw_text(font, cor, x, y, ALLEGRO_ALIGN_LEFT, linebuf);
            y += al_get_font_line_height(font);
        }

        // small paragraph gap
        if (y <= text_bottom) y += al_get_font_line_height(font) / 2;
    }

    return y;
}

// Caixa simples do saldo do jogador
void draw_player_balance_box(int x, int y, int w, int h)
{
    retangulo_preenchido(x, y, x + w, y + h, al_map_rgb(230, 230, 250));
    retangulo_bordas(x, y, x + w, y + h, al_map_rgb(0, 0, 0), 1.0);

    if ( fonte_linha ) texto(fonte_linha, "Saldo do Jogador", al_map_rgb(0,0,0), x + 8, y + 6, ALLEGRO_ALIGN_LEFT);

    char buf[64];
    snprintf(buf, sizeof(buf), "R$ %.2f", player_get_balance());
    if ( fonte_titulo ) texto(fonte_titulo, buf, al_map_rgb(0,100,0), x + 8, y + 30, ALLEGRO_ALIGN_LEFT);
}

// Mostra historico de transacoes (ultima linhas)
void draw_player_history_box(int x, int y, int w, int h, int max_lines)
{
    retangulo_preenchido(x, y, x + w, y + h, al_map_rgb(250, 250, 250));
    retangulo_bordas(x, y, x + w, y + h, al_map_rgb(0,0,0), 1.0);

    if ( fonte_linha ) texto(fonte_linha, "Historico de Gastos", al_map_rgb(0,0,0), x + 8, y + 6, ALLEGRO_ALIGN_LEFT);

    int qtd = player_history_count();
    int inicio = qtd - max_lines; if ( inicio < 0 ) inicio = 0;
    int linha_h = 18; int ty = y + 30;

    for ( int i = inicio; i < qtd; ++i ) {
        const PlayerEntry* e = player_history_get(i);
        if ( ! e ) continue;
        char str[256];
        snprintf(str, sizeof(str), "%+.2f  %s", e->amount, e->desc);
        if ( fonte_pequena ) texto(fonte_pequena, str, al_map_rgb(0,0,0), x + 8, ty, ALLEGRO_ALIGN_LEFT);
        ty += linha_h; if ( ty > y + h - 8 ) break;
    }
}

// Mostra portfolio e P/L nao realizado (simplificado)
void draw_player_portfolio_box(int x, int y, int w, int h, int max_lines)
{
    retangulo_preenchido(x, y, x + w, y + h, al_map_rgb(245, 255, 245));
    retangulo_bordas(x, y, x + w, y + h, al_map_rgb(0,0,0), 1.0);

    if ( fonte_linha ) texto(fonte_linha, "Portfolio", al_map_rgb(0,0,0), x + 8, y + 6, ALLEGRO_ALIGN_LEFT);

    int qtd = player_portfolio_count();
    int lh = 20; int ty = y + 34;

    // Show simple list: "Nome xQty   Pago: R$xx.xx   Atual: R$yy.yy"
    for ( int i = 0; i < qtd && i < max_lines; ++i ) {
        const PlayerHolding* p = player_portfolio_get(i);
        if ( ! p ) continue;
        float atual = bolsas_get_price_by_name(p->name);
        char line[256];
        if ( atual >= 0.0f ) {
            snprintf(line, sizeof(line), "%s x%d   Pago: R$%.2f   Atual: R$%.2f", p->name, p->qty, p->price, atual);
        } else {
            snprintf(line, sizeof(line), "%s x%d   Pago: R$%.2f   Atual: --", p->name, p->qty, p->price);
        }
        if ( fonte_pequena ) texto(fonte_pequena, line, al_map_rgb(0,0,0), x + 8, ty, ALLEGRO_ALIGN_LEFT);
        ty += lh; if ( ty > y + h - 12 ) break;
    }

    // summary line
    float total_pl = 0.0f;
    for ( int i = 0; i < qtd; ++i ) {
        const PlayerHolding* p = player_portfolio_get(i);
        if ( ! p ) continue;
        float atual = bolsas_get_price_by_name(p->name);
        if ( atual >= 0.0f ) total_pl += (atual - p->price) * p->qty;
    }
    char fbuf[128]; snprintf(fbuf, sizeof(fbuf), "Total P/L: R$ %.2f", total_pl);
    if ( fonte_linha ) texto(fonte_linha, fbuf, al_map_rgb(total_pl >= 0 ? 0 : 200, 0, 0), x + 8, y + h - 18, ALLEGRO_ALIGN_LEFT);
}

// Verifica se ponto esta dentro de retangulo (inteiros)
static int dentro(int mx, int my, int x1, int y1, int x2, int y2) {
    return ( mx >= x1 && mx <= x2 && my >= y1 && my <= y2 );
}

// Atualiza indice de hover com base na posicao do mouse
void investir_handle_mouse_move(int mx, int my)
{
    int x = 140, y = 180, x2 = 1140;
    int title_h = 34; // same as draw

    // compute right panel and left list so they nearly touch
    int right_w = 340;
    int right_x = x2 - right_w - 10; // 10px margin from window edge
    int left_x1 = x + 10;
    int left_x2 = right_x - 12; // small gap to the right panel

    int list_y = y + title_h + 8 + 28; // tab area + spacing (matches draw)
    int row_h = 34; int row_y = list_y + 30;
    bolsa_hover = -1;
    int qtd = bolsas_count();

    if ( mx >= left_x1 + 2 && mx <= left_x2 - 2 && my >= row_y ) {
        for ( int i = 0; i < qtd; ++i ) {
            int ry = row_y + i * ( row_h + 6 );
            if ( my >= ry && my <= ry + row_h ) { bolsa_hover = i; break; }
        }
    }
}

// Trata clique do mouse na tela de investimento
void investir_handle_click(int mx, int my)
{
    // if story modal open, consume clicks and let modal handler close it
    if ( tela_invest_is_story_modal_open() ) {
        if ( tela_invest_handle_modal_click(mx, my) ) return;
        // consume any clicks while modal is open
        return;
    }

    int x = 140, y = 180, x2 = 1140, y2 = 780;
    int title_h = 34; // match draw
    int tab_w = 140, tab_h = 28; int tab_y = y + title_h + 8;
    int tab1_x = x + 12; int tab2_x = tab1_x + tab_w + 8; int tab3_x = tab2_x + tab_w + 8;
    int tab4_x = tab3_x + tab_w + 8; int tab5_x = tab4_x + tab_w + 8;

    // Tabs click: Investimentos / Mae / Noticias / Tutorial / Dicas
    if ( dentro(mx, my, tab1_x, tab_y, tab1_x + tab_w, tab_y + tab_h) ) { main_aba = 0; bolsa_selecionada = -1; return; }
    if ( dentro(mx, my, tab2_x, tab_y, tab2_x + tab_w, tab_y + tab_h) ) { main_aba = 1; bolsa_selecionada = -1; return; }
    if ( dentro(mx, my, tab3_x, tab_y, tab3_x + tab_w, tab_y + tab_h) ) { main_aba = 2; bolsa_selecionada = -1; return; }
    if ( dentro(mx, my, tab4_x, tab_y, tab4_x + tab_w, tab_y + tab_h) ) { main_aba = 3; bolsa_selecionada = -1; return; }
    if ( dentro(mx, my, tab5_x, tab_y, tab5_x + tab_w, tab_y + tab_h) ) { main_aba = 4; bolsa_selecionada = -1; return; }

    // If mother died, show game over overlay; handle its button click
    if ( tela_invest_is_mother_dead() || tela_invest_get_mae_saude() <= 0.0f ) {
        // overlay button centered in the tela_invest area
        int btn_w = 260, btn_h = 48;
        int bx = (x + x2)/2 - btn_w/2;
        int by = (y + y2)/2 + 60;
        if ( dentro(mx, my, bx, by, bx + btn_w, by + btn_h) ) {
            // request main loop to return to menu
            tela_invest_request_return_to_menu();
        }
        // consume clicks when mother dead
        return;
    }

    // if game finished (other endings), ignore clicks
    if ( tela_invest_is_finished() ) return;

    // Special handling for Mae tab: interactive treatment buttons
    if ( main_aba == 1 ) {
        // recompute layout for Mae tab
        int right_w = 340;
        int right_x = x2 - right_w - 10;
        int left_x1 = x + 10;
        int left_x2 = right_x - 12;
        int list_y = tab_y + tab_h + 8;
        int list_h = y2 - list_y - 80;

        int bw_btn = 140, bh_btn = 38;
        int spacing = 12;
        int total_w = bw_btn * 3 + spacing * 2;
        int start_x = (left_x1 + left_x2) / 2 - total_w / 2;
        int btn_y = list_y + list_h - bh_btn - 12;

        // Remedio
        if ( dentro(mx, my, start_x, btn_y, start_x + bw_btn, btn_y + bh_btn) ) {
            float cost = 50.0f;
            if ( player_get_balance() >= cost ) {
                player_add_expense(cost, "Remedio");
                tela_invest_apply_treatment(5.0f);
            } else {
                tela_invest_open_story_modal("Saldo insuficiente para Remedio.");
            }
            return;
        }
        // Visita
        int visita_x = start_x + (bw_btn + spacing);
        if ( dentro(mx, my, visita_x, btn_y, visita_x + bw_btn, btn_y + bh_btn) ) {
            float cost = 120.0f;
            if ( player_get_balance() >= cost ) {
                player_add_expense(cost, "Visita");
                tela_invest_apply_treatment(15.0f);
            } else {
                tela_invest_open_story_modal("Saldo insuficiente para Visita.");
            }
            return;
        }
        // Emergencia
        int emerg_x = start_x + 2 * (bw_btn + spacing);
        if ( dentro(mx, my, emerg_x, btn_y, emerg_x + bw_btn, btn_y + bh_btn) ) {
            float cost = 400.0f;
            if ( player_get_balance() >= cost ) {
                player_add_expense(cost, "Emergencia");
                tela_invest_apply_treatment(50.0f);
            } else {
                tela_invest_open_story_modal("Saldo insuficiente para Emergencia.");
            }
            return;
        }

        // Also handle PASSAR DIA button on the right if clicked
        int btn_dia_w = 120, btn_dia_h = 35; int btn_dia_x = x2 - 150; int btn_dia_y = y2 - 50;
        if ( dentro(mx, my, btn_dia_x, btn_dia_y, btn_dia_x + btn_dia_w, btn_dia_y + btn_dia_h) ) {
            tela_invest_next_day(); bolsas_next_day();
            return;
        }

        // ignore other clicks in Mae tab
        return;
    }

    // Otherwise proceed with investments area handling
    if ( main_aba != 0 ) return; // if not investments, ignore

    // recompute layout (must match draw)
    int right_w = 340;
    int right_x = x2 - right_w - 10;
    int left_x1 = x + 10;
    int left_x2 = right_x - 12;
    int list_y = tab_y + tab_h + 8;
    int row_h = 34; int row_y = list_y + 30;

    int qtd = bolsas_count();
    // selecao na lista
    if ( mx >= left_x1 + 2 && mx <= left_x2 - 2 && my >= row_y ) {
        for ( int i = 0; i < qtd; ++i ) {
            int ry = row_y + i * ( row_h + 6 );
            if ( my >= ry && my <= ry + row_h ) { bolsa_selecionada = i; return; }
        }
    }

    // Botao PASSAR DIA
    int btn_dia_w = 120, btn_dia_h = 35;
    int btn_dia_x = x2 - 150;
    int btn_dia_y = y2 - 50;
    if ( dentro(mx, my, btn_dia_x, btn_dia_y, btn_dia_x + btn_dia_w, btn_dia_y + btn_dia_h) ) {
        // advance both game day and market prices
        tela_invest_next_day();
        bolsas_next_day();
        bolsa_selecionada = -1;
        return;
    }

    // Static Buy/Sell button positions (match drawing)
    int list_h = y2 - list_y - 80;
    int btn_y = list_y + list_h + 12; // fixed position below the list
    int bw = 140, bh = 38;
    int center_x = (left_x1 + left_x2) / 2;
    int bx = center_x - bw - 10;
    int sx = center_x + 10;

    // check buy click
    if ( dentro(mx, my, bx, btn_y, bx + bw, btn_y + bh) ) {
        if ( bolsa_selecionada >= 0 && bolsa_selecionada < qtd ) {
            const Bolsa* b = bolsas_get(bolsa_selecionada);
            if ( b ) player_buy_stock(b->nome, b->preco, 1);
        }
        return;
    }

    // check sell click (only allowed if player owns the selected stock)
    if ( dentro(mx, my, sx, btn_y, sx + bw, btn_y + bh) ) {
        if ( bolsa_selecionada >= 0 && bolsa_selecionada < qtd ) {
            const Bolsa* b = bolsas_get(bolsa_selecionada);
            if ( b ) {
                int pc = player_portfolio_count();
                int can_sell = 0;
                for ( int pi = 0; pi < pc; ++pi ) {
                    const PlayerHolding* ph = player_portfolio_get(pi);
                    if ( ph && ph->name && strcmp(ph->name, b->nome) == 0 && ph->qty > 0 ) { can_sell = 1; break; }
                }
                if ( can_sell ) player_sell_stock(b->nome, b->preco, 1);
            }
        }
        return;
    }
}

// Funcoes de desenho padrao (menu, tela invest) -------------------------------------------------
void aba_padrao(int x, int y, int x2, int y2) {
    retangulo_bordas(x - 1, y + 1, x2 + 1, y2 + 1, al_map_rgb(51, 25, 0), 2.0);
    retangulo_bordas(x - 1, y - 1, x2 - 1, y2 - 1, al_map_rgb(255, 255, 255), 2.0);
    retangulo_preenchido(x, y, x2, y2, al_map_rgb(192, 192, 192));
}

void barra_de_tarefas() {
    retangulo_bordas(0, 919, 1280, 960, al_map_rgb(255, 255, 255), 2.0);
    retangulo_preenchido(0, 920, 1280, 960, al_map_rgb(192, 192, 192));
}

void tela_menu(int x, int y, int x2, int y2) {
    retangulo_bordas(x - 1, y + 1, x2 + 1, y2 + 1, al_map_rgb(51, 25, 0), 2.0);
    retangulo_bordas(x - 1, y - 1, x2 - 1, y2 - 1, al_map_rgb(255, 255, 255), 2.0);
    retangulo_preenchido(x, y, x2, y2, al_map_rgb(192, 192, 192));
    retangulo_preenchido(x + 1, y, x2 - 1, y + 35, al_map_rgb(10, 15, 122));
    linha(x + 2, y + 60, x2 - 2, y + 60, al_map_rgb(0, 0, 0), 1.0);
    retangulo_preenchido(x + 10, y + 70, x2 - 10, y + 200, al_map_rgb(255, 255, 255));
    aba_padrao(x + 10, y + 210, x2 - 10, y + 250);
    aba_padrao(x + 10, y + 270, x2 - 10, y + 310);
    aba_padrao(x + 10, y + 330, x2 - 10, y + 370);
    linha(x + 2, y + 390, x2 - 2, y + 390, al_map_rgb(0, 0, 0), 1.0);
    linha(x + 2, y2 - 40, x2 - 2, y2 - 40, al_map_rgb(0, 0, 0), 1.0);
    ALLEGRO_FONT* f = al_load_ttf_font("arial.ttf", 24, 0);
    if ( f ) { texto(f, "CashFlow - 2000 - Menu Principal", al_map_rgb(255,255,255), x + 5, y + 5, ALLEGRO_ALIGN_LEFT); texto(f, "CASH FLOW 2000", al_map_rgb(255,200,0), (x + x2)/2, y + 110, ALLEGRO_ALIGN_CENTRE); al_destroy_font(f); }
    ALLEGRO_FONT* fo = al_load_ttf_font("arial.ttf", 20, 0);
    if ( fo ) { texto(fo, "Novo Jogo - (A)", al_map_rgb(0,0,0), (x + x2)/2, y + 215, ALLEGRO_ALIGN_CENTRE); texto(fo, "Continuar - (ENTER)", al_map_rgb(0,0,0), (x + x2)/2, y + 275, ALLEGRO_ALIGN_CENTRE); texto(fo, "Sair - (ESC)", al_map_rgb(0,0,0), (x + x2)/2, y + 335, ALLEGRO_ALIGN_CENTRE); al_destroy_font(fo); }
}

void tela_invest(int x, int y, int x2, int y2)
{
    retangulo_bordas(x - 1, y - 1, x2 - 1, y2 - 1, al_map_rgb(255, 255, 255), 2.0);
    retangulo_bordas(x + 1, y + 1, x2 + 1, y2 + 1, al_map_rgb(51, 25, 0), 2.0);
    retangulo_preenchido(x, y, x2, y2, al_map_rgb(192, 192, 192));

    int title_h = 34; // header maior
    retangulo_preenchido(x + 1, y, x2 - 1, y + title_h, al_map_rgb(50, 100, 180));
    if ( fonte_titulo ) texto(fonte_titulo, "Cash Flow 2000 - Aba Investimentos", al_map_rgb(255,255,255), x + 8, y + 6, ALLEGRO_ALIGN_LEFT);

    // Day counter box centered above content
    int day_box_w = 180, day_box_h = 44;
    int day_box_x = (x + x2)/2 - day_box_w/2;
    int day_box_y = y - day_box_h/2 + 10;
    retangulo_preenchido(day_box_x, day_box_y, day_box_x + day_box_w, day_box_y + day_box_h, al_map_rgb(20, 70, 140));
    retangulo_bordas(day_box_x, day_box_y, day_box_x + day_box_w, day_box_y + day_box_h, al_map_rgb(255,255,255), 2.0);
    char buf[64]; snprintf(buf, sizeof(buf), "Dia %d / %d", tela_invest_get_day(), tela_invest_get_max_days());
    if ( fonte_titulo ) texto(fonte_titulo, buf, al_map_rgb(255,255,255), day_box_x + day_box_w/2, day_box_y + 10, ALLEGRO_ALIGN_CENTRE);

    // Health indicator box (top-right corner, separate)
    {
        float mae_val = tela_invest_get_mae_saude();
        int hb_w = 200, hb_h = 44;
        int hb_x = x2 - hb_w - 12; // 12px margin from right
        int hb_y = y + 6; // inside header area
        retangulo_preenchido(hb_x, hb_y, hb_x + hb_w, hb_y + hb_h, al_map_rgb(30, 70, 120));
        retangulo_bordas(hb_x, hb_y, hb_x + hb_w, hb_y + hb_h, al_map_rgb(255,255,255), 2.0);
        // percentage text larger on left of box
        char pct[64]; snprintf(pct, sizeof(pct), "Saude: %.0f%%", mae_val);
        if ( fonte_titulo ) texto(fonte_titulo, pct, al_map_rgb(255,255,255), hb_x + 10, hb_y + 6, ALLEGRO_ALIGN_LEFT);
        // small bar below
        int inner_x = hb_x + 10;
        int inner_w = hb_w - 20;
        int inner_h = 12;
        int inner_y = hb_y + hb_h - inner_h - 8;
        retangulo_preenchido(inner_x, inner_y, inner_x + inner_w, inner_y + inner_h, al_map_rgb(200,200,200));
        int filled = (int)(inner_w * (mae_val / 100.0f)); if (filled < 0) filled = 0; if (filled > inner_w) filled = inner_w;
        ALLEGRO_COLOR bar_col = mae_val > 75 ? al_map_rgb(0,200,0) : (mae_val > 50 ? al_map_rgb(255,200,0) : al_map_rgb(200,0,0));
        retangulo_preenchido(inner_x, inner_y, inner_x + filled, inner_y + inner_h, bar_col);
    }

    // Tabs
    int tab_w = 140, tab_h = 28; int tab_y = y + title_h + 8;
    int tab1_x = x + 12; int tab2_x = tab1_x + tab_w + 8; int tab3_x = tab2_x + tab_w + 8;
    int tab4_x = tab3_x + tab_w + 8; int tab5_x = tab4_x + tab_w + 8;
    ALLEGRO_COLOR c1 = (main_aba==0) ? al_map_rgb(100,150,220) : al_map_rgb(180,180,180);
    ALLEGRO_COLOR c2 = (main_aba==1) ? al_map_rgb(100,150,220) : al_map_rgb(180,180,180);
    ALLEGRO_COLOR c3 = (main_aba==2) ? al_map_rgb(100,150,220) : al_map_rgb(180,180,180);
    ALLEGRO_COLOR c4 = (main_aba==3) ? al_map_rgb(100,150,220) : al_map_rgb(180,180,180);
    ALLEGRO_COLOR c5 = (main_aba==4) ? al_map_rgb(100,150,220) : al_map_rgb(180,180,180);
    retangulo_preenchido(tab1_x, tab_y, tab1_x + tab_w, tab_y + tab_h, c1); retangulo_bordas(tab1_x, tab_y, tab1_x + tab_w, tab_y + tab_h, al_map_rgb(0,0,0), 1.0);
    if ( fonte_pequena ) texto(fonte_pequena, "Investimentos", al_map_rgb(255,255,255), tab1_x + tab_w/2, tab_y + 6, ALLEGRO_ALIGN_CENTRE);
    retangulo_preenchido(tab2_x, tab_y, tab2_x + tab_w, tab_y + tab_h, c2); retangulo_bordas(tab2_x, tab_y, tab2_x + tab_w, tab_y + tab_h, al_map_rgb(0,0,0), 1.0);
    if ( fonte_pequena ) texto(fonte_pequena, "Mae", al_map_rgb(255,255,255), tab2_x + tab_w/2, tab_y + 6, ALLEGRO_ALIGN_CENTRE);
    retangulo_preenchido(tab3_x, tab_y, tab3_x + tab_w, tab_y + tab_h, c3); retangulo_bordas(tab3_x, tab_y, tab3_x + tab_w, tab_y + tab_h, al_map_rgb(0,0,0), 1.0);
    if ( fonte_pequena ) texto(fonte_pequena, "Noticias", al_map_rgb(255,255,255), tab3_x + tab_w/2, tab_y + 6, ALLEGRO_ALIGN_CENTRE);
    retangulo_preenchido(tab4_x, tab_y, tab4_x + tab_w, tab_y + tab_h, c4); retangulo_bordas(tab4_x, tab_y, tab4_x + tab_w, tab_y + tab_h, al_map_rgb(0,0,0), 1.0);
    if ( fonte_pequena ) texto(fonte_pequena, "Tutorial", al_map_rgb(255,255,255), tab4_x + tab_w/2, tab_y + 6, ALLEGRO_ALIGN_CENTRE);
    retangulo_preenchido(tab5_x, tab_y, tab5_x + tab_w, tab_y + tab_h, c5); retangulo_bordas(tab5_x, tab_y, tab5_x + tab_w, tab_y + tab_h, al_map_rgb(0,0,0), 1.0);
    if ( fonte_pequena ) texto(fonte_pequena, "Dicas", al_map_rgb(255,255,255), tab5_x + tab_w/2, tab_y + 6, ALLEGRO_ALIGN_CENTRE);

    // compute right panel and left list so they nearly touch
    int right_w = 340;
    int right_x = x2 - right_w - 10; // 10px margin from window edge
    int left_x1 = x + 10;
    int left_x2 = right_x - 12; // small gap to the right panel

    // decide o conteudo
    if ( main_aba == 0 ) {
        // draw investimentos area (unchanged)
        int list_y = tab_y + tab_h + 8; int list_h = y2 - list_y - 80;
        retangulo_preenchido(left_x1, list_y, left_x2, list_y + list_h, al_map_rgb(255,255,255));
        retangulo_bordas(left_x1, list_y, left_x2, list_y + list_h, al_map_rgb(0,0,0), 1.0);

        // compute column positions so price and variation do not overlap
        int col_name_x = left_x1 + 12;
        int risk_size = 12; // size of the small risk square
        // move columns more to the left to avoid cramping on the right
        int col_risk_x = left_x2 - 120;            // moved left from -60
        int col_delta_center = left_x2 - 220;      // moved left from -160 (center for variation column)
        int col_price_x = left_x2 - 360;           // moved left from -300

        if ( fonte_linha ) {
            texto(fonte_linha, "Bolsa", al_map_rgb(0,0,0), col_name_x, list_y + 6, ALLEGRO_ALIGN_LEFT);
            texto(fonte_linha, "Preco", al_map_rgb(0,0,0), col_price_x, list_y + 6, ALLEGRO_ALIGN_LEFT);
            texto(fonte_linha, "Variacao", al_map_rgb(0,0,0), col_delta_center, list_y + 6, ALLEGRO_ALIGN_CENTRE);
            // center the "Risco" header above the small colored square area
            texto(fonte_linha, "Risco", al_map_rgb(0,0,0), col_risk_x + risk_size/2, list_y + 6, ALLEGRO_ALIGN_CENTRE);
        }

        int qtd = bolsas_count(); int row_h = 34; int row_y = list_y + 30;
        double now = al_get_time();
        for ( int i = 0; i < qtd; ++i ) {
            const Bolsa* b = bolsas_get(i); if ( ! b ) continue; int ry = row_y + i * ( row_h + 6 );
            if ( i % 2 == 0 ) retangulo_preenchido(left_x1 + 2, ry, left_x2 - 2, ry + row_h, al_map_rgb(245,245,255)); else retangulo_preenchido(left_x1 + 2, ry, left_x2 - 2, ry + row_h, al_map_rgb(255,255,255));

            // Columns: name | price | delta | risk
            if ( fonte_linha ) texto(fonte_linha, b->nome, al_map_rgb(0,0,0), col_name_x, ry + 8, ALLEGRO_ALIGN_LEFT);

            // compute a small realtime oscillation for visual delta only
            float amplitude_frac = 0.003f; double freq = 0.6;
            if ( b->risco == 1 ) { amplitude_frac = 0.007f; freq = 0.9; }
            else if ( b->risco == 2 ) { amplitude_frac = 0.015f; freq = 1.4; }
            // add small index-based phase so each oscillates differently
            double phase = (double)(i % 7) * 0.73;
            double osc = sin(now * freq + phase) * (double)(b->preco * amplitude_frac);
            double display_price = (double)b->preco + osc;

            char preco_txt[64]; snprintf(preco_txt, sizeof(preco_txt), "R$ %.2f", (float)display_price);
            if ( fonte_linha ) texto(fonte_linha, preco_txt, al_map_rgb(0,0,0), col_price_x, ry + 8, ALLEGRO_ALIGN_LEFT);

            // delta (centered under header) - show display_price - last_preco
            double delta = display_price - (double)b->last_preco;
            char delta_txt[64];
            if ( delta >= 0 ) snprintf(delta_txt, sizeof(delta_txt), "+%.2f", (float)delta);
            else snprintf(delta_txt, sizeof(delta_txt), "%.2f", (float)delta);
            ALLEGRO_COLOR delta_col = delta >= 0 ? al_map_rgb(0,150,0) : al_map_rgb(200,0,0);
            if ( fonte_pequena ) texto(fonte_pequena, delta_txt, delta_col, col_delta_center, ry + 9, ALLEGRO_ALIGN_CENTRE);

            int risk_size2 = 12;
            int risk_x = col_risk_x;
            int risk_y = ry + ( row_h - risk_size2 ) / 2;
            ALLEGRO_COLOR risk_color = al_map_rgb(0,200,0);
            const char* risk_label = "Baixo";
            if ( b->risco == 1 ) { risk_color = al_map_rgb(240,180,0); risk_label = "Medio"; }
            if ( b->risco == 2 ) { risk_color = al_map_rgb(200,0,0); risk_label = "Alto"; }
            retangulo_preenchido(risk_x, risk_y, risk_x + risk_size2, risk_y + risk_size2, risk_color);
            if ( fonte_linha ) texto(fonte_linha, risk_label, al_map_rgb(0,0,0), risk_x + risk_size2 + 6, ry + 8, ALLEGRO_ALIGN_LEFT);

            if ( bolsa_hover == i ) retangulo_bordas(left_x1 + 2, ry, left_x2 - 2, ry + row_h, al_map_rgb(180,180,0), 1.0);
            if ( bolsa_selecionada == i ) retangulo_bordas(left_x1 + 2, ry, left_x2 - 2, ry + row_h, al_map_rgb(255,215,0), 3.0);
        }

        // Static Buy/Sell buttons placed below the bolsa list (always visible)
        int bw_btn = 140, bh_btn = 38;
        int btn_y = list_y + list_h + 12; // fixed position below the list
        int center_x = (left_x1 + left_x2) / 2;
        int bx_btn = center_x - bw_btn - 10;
        int sx_btn = center_x + 10;

        int qtd_pos = bolsas_count();
        int has_sel = (bolsa_selecionada >= 0 && bolsa_selecionada < qtd_pos);

        // determine if player owns selected stock
        int can_sell = 0;
        if ( has_sel ) {
            const Bolsa* sb = bolsas_get(bolsa_selecionada);
            int pc = player_portfolio_count();
            for ( int pi = 0; pi < pc; ++pi ) {
                const PlayerHolding* ph = player_portfolio_get(pi);
                if ( ph && ph->name && sb && strcmp(ph->name, sb->nome) == 0 && ph->qty > 0 ) { can_sell = 1; break; }
            }
        }

        // Draw BUY button
        if ( has_sel && player_get_balance() >= 0.0f ) retangulo_preenchido(bx_btn, btn_y, bx_btn + bw_btn, btn_y + bh_btn, al_map_rgb(0,150,100));
        else retangulo_preenchido(bx_btn, btn_y, bx_btn + bw_btn, btn_y + bh_btn, al_map_rgb(180,180,180));
        retangulo_bordas(bx_btn, btn_y, bx_btn + bw_btn, btn_y + bh_btn, al_map_rgb(0,0,0), 2.0);
        if ( fonte_pequena ) texto(fonte_pequena, "COMPRAR", has_sel ? al_map_rgb(255,255,255) : al_map_rgb(120,120,120), bx_btn + bw_btn/2, btn_y + 8, ALLEGRO_ALIGN_CENTRE);

        // Draw SELL button (disabled if player doesn't own the selected stock)
        if ( has_sel && can_sell ) retangulo_preenchido(sx_btn, btn_y, sx_btn + bw_btn, btn_y + bh_btn, al_map_rgb(180,0,0));
        else retangulo_preenchido(sx_btn, btn_y, sx_btn + bw_btn, btn_y + bh_btn, al_map_rgb(160,160,160));
        retangulo_bordas(sx_btn, btn_y, sx_btn + bw_btn, btn_y + bh_btn, al_map_rgb(0,0,0), 2.0);
        if ( fonte_pequena ) texto(fonte_pequena, "VENDER", (has_sel && can_sell) ? al_map_rgb(255,255,255) : al_map_rgb(120,120,120), sx_btn + bw_btn/2, btn_y + 8, ALLEGRO_ALIGN_CENTRE);

        // Painel direito: show balance, history and portfolio stacked (no tabs)
        int right_w_local = right_w;
        draw_player_balance_box(right_x, list_y, right_w_local, 80);

        int content_top = list_y + 88;
        int content_h = y2 - content_top - 80; // leave space at bottom for controls
        if (content_h < 80) content_h = 80;
        int history_h = (content_h - 12) / 2;
        int portfolio_h = content_h - history_h - 12;

        draw_player_history_box(right_x, content_top, right_w_local, history_h, 12);
        draw_player_portfolio_box(right_x, content_top + history_h + 12, right_w_local, portfolio_h, 12);

        // Botão PASSAR DIA
        int btn_dia_w = 120, btn_dia_h = 35; int btn_dia_x = x2 - 150; int btn_dia_y = y2 - 50; retangulo_preenchido(btn_dia_x, btn_dia_y, btn_dia_x + btn_dia_w, btn_dia_y + btn_dia_h, al_map_rgb(50, 100, 180)); retangulo_bordas(btn_dia_x, btn_dia_y, btn_dia_x + btn_dia_w, btn_dia_y + btn_dia_h, al_map_rgb(100, 150, 220), 2.5); if ( fonte_linha ) texto(fonte_linha, "PASSAR DIA", al_map_rgb(255, 255, 255), btn_dia_x + btn_dia_w/2, btn_dia_y + 8, ALLEGRO_ALIGN_CENTRE);

        // Info dia
        char info_text[128]; int dia = tela_invest_get_day(); float mae_val = tela_invest_get_mae_saude();
        snprintf(info_text, sizeof(info_text), "Dia: %d/%d", dia, tela_invest_get_max_days()); if ( fonte_pequena ) texto(fonte_pequena, info_text, al_map_rgb(0,0,0), left_x1 + 10, y2 - 40, ALLEGRO_ALIGN_LEFT);

    } else if ( main_aba == 1 ) {
        // Mae tab: health and treatments (with interactive buttons and embedded health bar)
        int list_y = tab_y + tab_h + 8; int list_h = y2 - list_y - 80;
        retangulo_preenchido(left_x1, list_y, left_x2, list_y + list_h, al_map_rgb(250,245,240));
        retangulo_bordas(left_x1, list_y, left_x2, list_y + list_h, al_map_rgb(0,0,0), 1.0);
        if ( fonte_linha ) texto(fonte_linha, "Mae - Saude e Tratamentos", al_map_rgb(0,0,0), left_x1 + 8, list_y + 6, ALLEGRO_ALIGN_LEFT);

        int ty = list_y + 36; int lh = 22;
        char buf2[128];
        float mae_val = tela_invest_get_mae_saude();
        snprintf(buf2, sizeof(buf2), "Saude atual: %.0f%%", mae_val);
        if ( fonte_pequena ) texto(fonte_pequena, buf2, al_map_rgb(10,10,10), left_x1 + 12, ty, ALLEGRO_ALIGN_LEFT);
        ty += lh;

        const char* descr[] = {
            "Tratamentos disponiveis:",
            "1) Remedio - Custa R$50 - Restaura +5%% de saude (imediato).",
            "2) Visita  - Custa R$120 - Restaura +15%% de saude (imediato).",
            "3) Emergencia - Custa R$400 - Restaura +50%% de saude (imediato).",
            "Reserve sempre uma parte do saldo para tratamentos; perder a mae acaba o jogo."
        };
        int nd = sizeof(descr)/sizeof(descr[0]);
        for ( int i = 0; i < nd; ++i ) {
            if ( fonte_pequena ) texto(fonte_pequena, descr[i], al_map_rgb(20,20,20), left_x1 + 12, ty, ALLEGRO_ALIGN_LEFT);
            ty += lh; if ( ty > list_y + list_h - 60 ) break;
        }

        // draw embedded health bar inside Mae tab
        int bar_x = left_x1 + 12;
        int bar_w = left_x2 - left_x1 - 24;
        int bar_h = 14;
        int bar_y = ty;
        retangulo_preenchido(bar_x, bar_y, bar_x + bar_w, bar_y + bar_h, al_map_rgb(200,200,200));
        int filled = (int)(bar_w * (mae_val / 100.0f)); if (filled < 0) filled = 0; if (filled > bar_w) filled = bar_w;
        ALLEGRO_COLOR bar_col = mae_val > 75 ? al_map_rgb(0,200,0) : (mae_val > 50 ? al_map_rgb(255,200,0) : al_map_rgb(200,0,0));
        retangulo_preenchido(bar_x, bar_y, bar_x + filled, bar_y + bar_h, bar_col);
        ty += bar_h + 12;

        // Buttons for buying treatments (aligned centered at bottom of left panel)
        int btn_w = 140, btn_h = 38, spacing = 12;
        int total_w = btn_w * 3 + spacing * 2;
        int start_x = (left_x1 + left_x2) / 2 - total_w / 2;
        int btn_y = list_y + list_h - btn_h - 12;

        // Remedio button
        int rx = start_x;
        int ry = btn_y;
        int can_rem = (player_get_balance() >= 50.0f);
        if ( can_rem ) retangulo_preenchido(rx, ry, rx + btn_w, ry + btn_h, al_map_rgb(0,150,100)); else retangulo_preenchido(rx, ry, rx + btn_w, ry + btn_h, al_map_rgb(180,180,180));
        retangulo_bordas(rx, ry, rx + btn_w, ry + btn_h, al_map_rgb(0,0,0), 2.0);
        if ( fonte_pequena ) { texto(fonte_pequena, "REMEDIO", can_rem ? al_map_rgb(255,255,255) : al_map_rgb(120,120,120), rx + btn_w/2, ry + 6, ALLEGRO_ALIGN_CENTRE); texto(fonte_pequena, "R$50", can_rem ? al_map_rgb(255,255,255) : al_map_rgb(120,120,120), rx + btn_w/2, ry + 20, ALLEGRO_ALIGN_CENTRE); }

        // Visita button
        int vx = start_x + (btn_w + spacing);
        int vy = btn_y;
        int can_vis = (player_get_balance() >= 120.0f);
        if ( can_vis ) retangulo_preenchido(vx, vy, vx + btn_w, vy + btn_h, al_map_rgb(50,120,200)); else retangulo_preenchido(vx, vy, vx + btn_w, vy + btn_h, al_map_rgb(180,180,180));
        retangulo_bordas(vx, vy, vx + btn_w, vy + btn_h, al_map_rgb(0,0,0), 2.0);
        if ( fonte_pequena ) { texto(fonte_pequena, "VISITA", can_vis ? al_map_rgb(255,255,255) : al_map_rgb(120,120,120), vx + btn_w/2, vy + 6, ALLEGRO_ALIGN_CENTRE); texto(fonte_pequena, "R$120", can_vis ? al_map_rgb(255,255,255) : al_map_rgb(120,120,120), vx + btn_w/2, vy + 20, ALLEGRO_ALIGN_CENTRE); }

        // Emergencia button
        int ex = start_x + 2 * (btn_w + spacing);
        int ey = btn_y;
        int can_emg = (player_get_balance() >= 400.0f);
        if ( can_emg ) retangulo_preenchido(ex, ey, ex + btn_w, ey + btn_h, al_map_rgb(200,100,0)); else retangulo_preenchido(ex, ey, ex + btn_w, ey + btn_h, al_map_rgb(180,180,180));
        retangulo_bordas(ex, ey, ex + btn_w, ey + btn_h, al_map_rgb(0,0,0), 2.0);
        if ( fonte_pequena ) { texto(fonte_pequena, "EMERGENCIA", can_emg ? al_map_rgb(255,255,255) : al_map_rgb(120,120,120), ex + btn_w/2, ey + 6, ALLEGRO_ALIGN_CENTRE); texto(fonte_pequena, "R$400", can_emg ? al_map_rgb(255,255,255) : al_map_rgb(120,120,120), ex + btn_w/2, ey + 20, ALLEGRO_ALIGN_CENTRE); }

        // right panel still shows balance/history/portfolio
        int right_w_local = right_w;
        draw_player_balance_box(right_x, list_y, right_w_local, 80);
        int content_top = list_y + 88;
        int content_h = y2 - content_top - 80; if (content_h < 80) content_h = 80;
        int history_h = (content_h - 12) / 2; int portfolio_h = content_h - history_h - 12;
        draw_player_history_box(right_x, content_top, right_w_local, history_h, 12);
        draw_player_portfolio_box(right_x, content_top + history_h + 12, right_w_local, portfolio_h, 12);

    } else if ( main_aba == 2 ) {
        // Noticias tab: show today's selected noticias and a preview for next day
        int list_y = tab_y + tab_h + 8; int list_h = y2 - list_y - 80;
        retangulo_preenchido(left_x1, list_y, left_x2, list_y + list_h, al_map_rgb(245,245,255));
        retangulo_bordas(left_x1, list_y, left_x2, list_y + list_h, al_map_rgb(0,0,0), 1.0);
        if ( fonte_linha ) texto(fonte_linha, "Noticias - Eventos aplicados hoje e previsao para amanha", al_map_rgb(0,0,0), left_x1 + 8, list_y + 6, ALLEGRO_ALIGN_LEFT);

        int ty = list_y + 36; int lh = 20;
        int day = tela_invest_get_day();
        // picks for today
        int count_today = 1 + (day % 3);
        int picks_today[4] = { -1, -1, -1, -1 };
        noticias_pick_for_day(day, picks_today, count_today);
        if ( fonte_pequena ) texto(fonte_pequena, "Aplicadas hoje:", al_map_rgb(30,30,30), left_x1 + 12, ty, ALLEGRO_ALIGN_LEFT);
        ty += lh;
        for ( int i = 0; i < count_today; ++i ) {
            int idx = picks_today[i];
            const Noticia* n = noticias_get(idx);
            if ( ! n ) continue;
            char title[256];
            float imp_pct = n->impacto * 100.0f;
            snprintf(title, sizeof(title), "- %s (Impacto: %+0.0f%%)", n->titulo ? n->titulo : "", imp_pct);
            if ( fonte_pequena ) texto(fonte_pequena, title, al_map_rgb(20,20,20), left_x1 + 18, ty, ALLEGRO_ALIGN_LEFT);
            ty += lh;
            if ( n->texto && n->texto[0] != '\0' ) {
                if ( fonte_pequena ) texto(fonte_pequena, n->texto, al_map_rgb(100,100,100), left_x1 + 22, ty, ALLEGRO_ALIGN_LEFT);
                ty += lh;
            }
            if ( ty > list_y + list_h - 120 ) { if ( fonte_pequena ) texto(fonte_pequena, "... (mais noticias aplicadas)", al_map_rgb(80,80,80), left_x1 + 18, ty, ALLEGRO_ALIGN_LEFT); ty += lh; break; }
        }

        // preview for next day
        ty += 6;
        int next_day = day + 1;
        if ( next_day <= tela_invest_get_max_days() ) {
            int count_next = 1 + (next_day % 3);
            int picks_next[4] = { -1, -1, -1, -1 };
            noticias_pick_for_day(next_day, picks_next, count_next);
            if ( fonte_pequena ) texto(fonte_pequena, "Previsao para amanha (probabilidades estimadas):", al_map_rgb(30,30,30), left_x1 + 12, ty, ALLEGRO_ALIGN_LEFT);
            ty += lh;
            for ( int i = 0; i < count_next; ++i ) {
                int idx = picks_next[i];
                const Noticia* n = noticias_get(idx);
                if ( ! n ) continue;
                char line[512];
                // estimate probability of increase/decrease based on impacto and underlying bolsa risk
                float prob = 0.5f; // base
                const Bolsa* b = NULL;
                if ( n->bolsa_index >= 0 ) b = bolsas_get(n->bolsa_index);
                float impact = n->impacto; // can be negative
                int risco = b ? b->risco : 0;
                if ( impact >= 0.0f ) {
                    prob = 0.20f + impact * 0.5f - risco * 0.05f; // heuristic
                } else {
                    prob = 0.80f - fabsf(impact) * 0.5f - risco * 0.05f; // chance of decrease
                }
                if ( prob < 0.02f ) prob = 0.02f; if ( prob > 0.98f ) prob = 0.98f;
                int pct = (int)(prob * 100.0f + 0.5f);
                if ( impact >= 0.0f ) snprintf(line, sizeof(line), "- %s => Prob. de ALTA ~ %d%% (impacto ~ %+0.0f%%)", n->titulo ? n->titulo : "", pct, n->impacto*100.0f);
                else snprintf(line, sizeof(line), "- %s => Prob. de QUEDA ~ %d%% (impacto ~ %+0.0f%%)", n->titulo ? n->titulo : "", pct, n->impacto*100.0f);
                if ( fonte_pequena ) texto(fonte_pequena, line, al_map_rgb(40,40,120), left_x1 + 18, ty, ALLEGRO_ALIGN_LEFT);
                ty += lh;
                if ( ty > list_y + list_h - 40 ) { if ( fonte_pequena ) texto(fonte_pequena, "... (mais previsoes)", al_map_rgb(80,80,80), left_x1 + 18, ty, ALLEGRO_ALIGN_LEFT); ty += lh; break; }
            }
        }

        // right panel still shows balance/history/portfolio
        int right_w_local = right_w;
        draw_player_balance_box(right_x, list_y, right_w_local, 80);
        int content_top = list_y + 88;
        int content_h = y2 - content_top - 80; if (content_h < 80) content_h = 80;
        int history_h = (content_h - 12) / 2; int portfolio_h = content_h - history_h - 12;
        draw_player_history_box(right_x, content_top, right_w_local, history_h, 12);
        draw_player_portfolio_box(right_x, content_top + history_h + 12, right_w_local, portfolio_h, 12);

    } else if ( main_aba == 3 ) {
        // Tutorial tab: basic mechanics (use wrapped text to fit)
        int list_y = tab_y + tab_h + 8; int list_h = y2 - list_y - 80;
        retangulo_preenchido(left_x1, list_y, left_x2, list_y + list_h, al_map_rgb(250,250,245));
        retangulo_bordas(left_x1, list_y, left_x2, list_y + list_h, al_map_rgb(0,0,0), 1.0);
        if ( fonte_linha ) texto(fonte_linha, "Tutorial - Como Jogar", al_map_rgb(0,0,0), left_x1 + 8, list_y + 6, ALLEGRO_ALIGN_LEFT);

        int ty = list_y + 36;
        const char* lines[] = {
            "Objetivo: Chegar ao dia 100 com sua mae viva e juntar R$ 5000.",
            "Selecione uma bolsa na lista e use os botoes COMPRAR/VENDER para negociar (quantidade 1 por clique).",
            "Clique em PASSAR DIA para aplicar noticias e variacoes de preco; use isso para planejar entradas e saidas.",
            "Noticias podem causar boom ou queda; acompanhe o historico e use diversificacao para reduzir risco.",
            "Risco: 0=Baixo, 1=Medio, 2=Alto. Acoes de maior risco tem movimentos maiores e chances de crash/boom.",
            "Saude da mae: compre tratamentos (Remedios/Visita/Emergencia) para evitar GAME OVER. Sempre reserve caixa para isso.",
            "Use o painel direito para ver saldo, historico de transacoes e posicoes atuais; realize lucros quando necessario.",
            "Dica operacional: venda parcialmente quando um ativo subir muito; use pequenas posicoes em especulacao e maiores em ativos estaveis.",
            "Controle de risco: defina um limite de perda por posicao e nao reinvista tudo de uma vez; reequilibre periodicamente."
        };
        char combined[4096]; combined[0] = '\0';
        int nlines = sizeof(lines)/sizeof(lines[0]);
        for ( int i = 0; i < nlines; ++i ) {
            strncat(combined, lines[i], sizeof(combined) - strlen(combined) - 1);
            if (i < nlines - 1) strncat(combined, "\n", sizeof(combined) - strlen(combined) - 1);
        }
        int avail_h = (list_y + list_h) - ty - 20; if (avail_h < 0) avail_h = 0;
        ty = texto_draw_wrapped(fonte_pequena, combined, al_map_rgb(20,20,20), left_x1 + 12, ty, left_x2 - left_x1 - 24, avail_h);

        // right panel still shows balance/history/portfolio
        int right_w_local = right_w;
        draw_player_balance_box(right_x, list_y, right_w_local, 80);
        int content_top = list_y + 88;
        int content_h = y2 - content_top - 80; if (content_h < 80) content_h = 80;
        int history_h = (content_h - 12) / 2; int portfolio_h = content_h - history_h - 12;
        draw_player_history_box(right_x, content_top, right_w_local, history_h, 12);
        draw_player_portfolio_box(right_x, content_top + history_h + 12, right_w_local, portfolio_h, 12);

    } else if ( main_aba == 4 ) {
        // Dicas tab: strategies to reach R$5000 in 100 days (wrapped)
        int list_y = tab_y + tab_h + 8; int list_h = y2 - list_y - 80;
        retangulo_preenchido(left_x1, list_y, left_x2, list_y + list_h, al_map_rgb(245,250,245));
        retangulo_bordas(left_x1, list_y, left_x2, list_y + list_h, al_map_rgb(0,0,0), 1.0);
        if ( fonte_linha ) texto(fonte_linha, "Dicas - Alvo: R$5000 em 100 dias", al_map_rgb(0,0,0), left_x1 + 8, list_y + 6, ALLEGRO_ALIGN_LEFT);

        int ty = list_y + 36;
        const char* dicas[] = {
            "Contexto: Partindo de R$500 para R$5000 em 100 dias exige crescimento aproximado de 10x (alto risco).",
            "1) Alocacao: mantenha uma base de ativos de baixo risco (40-60%) para preservar capital.",
            "2) Especulacao: use 20-40% para posiciones de risco (memes) com potencial alto; limite o tamanho por trade.",
            "3) Protecao: reserve 10-20% em caixa para emergencias e tratamentos da mae; sem isso o jogo termina.",
            "4) Realize lucros: quando um ativo de especulacao subir forte (2x/3x), venda parcialmente para garantir ganhos.",
            "5) Rebalanceamento: semanalmente mova lucros de volta para ativos mais seguros e reduza exposicao em bombas.",
            "6) Posicionamento: use pequenas entradas escalonadas (dollar-cost averaging) em dips ao inves de tudo de uma vez.",
            "7) Gestao de risco: estabeleca um stop-loss mental; nao persiga perdas aumentando a posicao.",
            "8) Diversificacao: combine varios setores/ativos; evitar concentrar 100% do patrimonio em uma unica moeda meme.",
            "9) Aproveite noticias: entre cedo em ativos com noticias positivas e planeje saida antes de sinais de risco.",
            "10) Instrumentos estaveis: considere manter parte em ativos tipo \"SAFE\"/\"ACME\" que tendem a subir devagar e reduzir volatilidade.",
            "11) Disciplina: tenha metas claras (ex: alvo parcial de R$1500, R$3000) e tome decisoes baseadas em regra, nao em emocao.",
            "Observacao: Essas sao estrategias educacionais, nao garantem resultados. No jogo, o factor sorte e a volatilidade sao altos." 
        };
        char combined2[4096]; combined2[0] = '\0';
        int nd = sizeof(dicas)/sizeof(dicas[0]);
        for ( int i = 0; i < nd; ++i ) {
            strncat(combined2, dicas[i], sizeof(combined2) - strlen(combined2) - 1);
            if (i < nd - 1) strncat(combined2, "\n", sizeof(combined2) - strlen(combined2) - 1);
        }
        int avail_h2 = (list_y + list_h) - ty - 20; if (avail_h2 < 0) avail_h2 = 0;
        ty = texto_draw_wrapped(fonte_pequena, combined2, al_map_rgb(20,20,20), left_x1 + 12, ty, left_x2 - left_x1 - 24, avail_h2);

        // right panel still shows balance/history/portfolio
        int right_w_local = right_w;
        draw_player_balance_box(right_x, list_y, right_w_local, 80);
        int content_top = list_y + 88;
        int content_h = y2 - content_top - 80; if (content_h < 80) content_h = 80;
        int history_h = (content_h - 12) / 2; int portfolio_h = content_h - history_h - 12;
        draw_player_history_box(right_x, content_top, right_w_local, history_h, 12);
        draw_player_portfolio_box(right_x, content_top + history_h + 12, right_w_local, portfolio_h, 12);

    }

    // if story modal open or mother dead, draw overlays on top (mother-dead overlay first)
    if ( tela_invest_is_mother_dead() || tela_invest_get_mae_saude() <= 0.0f ) {
        // Draw full-screen semi-transparent overlay and centered GAME OVER text + button
        retangulo_preenchido(x - 6, y - 6, x2 + 6, y2 + 6, al_map_rgba(0,0,0,160));
        // big title
        ALLEGRO_FONT* fbig = al_load_ttf_font("arial.ttf", 64, 0);
        if (!fbig) fbig = fonte_titulo;
        texto(fbig, "GAME OVER", al_map_rgb(220,40,40), (x + x2)/2, (y + y2)/2 - 40, ALLEGRO_ALIGN_CENTRE);
        // small subtitle
        ALLEGRO_FONT* fsub = al_load_ttf_font("arial.ttf", 20, 0);
        if (!fsub) fsub = fonte_pequena;
        texto(fsub, "Sua mae morreu.", al_map_rgb(255,255,255), (x + x2)/2, (y + y2)/2 + 2, ALLEGRO_ALIGN_CENTRE);
        // button
        int btn_w = 260, btn_h = 48;
        int bx = (x + x2)/2 - btn_w/2;
        int by = (y + y2)/2 + 60;
        retangulo_preenchido(bx, by, bx + btn_w, by + btn_h, al_map_rgb(50,100,180));
        retangulo_bordas(bx, by, bx + btn_w, by + btn_h, al_map_rgb(255,255,255), 2.0);
        if ( fonte_linha ) texto(fonte_linha, "Voltar ao Menu", al_map_rgb(255,255,255), bx + btn_w/2, by + 12, ALLEGRO_ALIGN_CENTRE);
        if (fbig != fonte_titulo) al_destroy_font(fbig);
        if (fsub != fonte_pequena) al_destroy_font(fsub);

    } else if ( tela_invest_is_story_modal_open() ) {
        int win_w = 820; int win_h = 420;
        int win_x = (1280 - win_w) / 2;
        int win_y = (960 - win_h) / 2;

        // modal background and frame
        retangulo_preenchido(win_x - 6, win_y - 6, win_x + win_w + 6, win_y + win_h + 6, al_map_rgba(0,0,0,160));
        retangulo_preenchido(win_x, win_y, win_x + win_w, win_y + win_h, al_map_rgb(192,192,192));
        retangulo_bordas(win_x, win_y, win_x + win_w, win_y + win_h, al_map_rgb(0,0,0), 2.0);

        // title bar and close box
        retangulo_preenchido(win_x, win_y, win_x + win_w, win_y + 28, al_map_rgb(0,0,160));
        if ( fonte_titulo ) texto(fonte_titulo, "Historia - Cash Flow 2000", al_map_rgb(255,255,255), win_x + 8, win_y + 6, ALLEGRO_ALIGN_LEFT);
        int close_x = win_x + win_w - 28; int close_y = win_y + 6;
        retangulo_preenchido(close_x, close_y, close_x + 20, close_y + 20, al_map_rgb(200,50,50));
        retangulo_bordas(close_x, close_y, close_x + 20, close_y + 20, al_map_rgb(0,0,0), 1.0);
        if ( fonte_linha ) texto(fonte_linha, "X", al_map_rgb(255,255,255), close_x + 10, close_y + 2, ALLEGRO_ALIGN_CENTRE);

        // body text (wrapped)
        const char* story = tela_invest_get_story_text();
        ALLEGRO_FONT* modal_font = al_load_ttf_font("arial.ttf", 16, 0);
        ALLEGRO_FONT* use_font = modal_font ? modal_font : fonte_pequena;

        int text_x = win_x + 12;
        int text_y = win_y + 36;
        int text_w = win_w - 24;
        int text_bottom = win_y + win_h - 20;

        if (!use_font) {
            // fallback: draw raw lines
            const char* p = story;
            char line[512];
            while (*p && text_y <= text_bottom) {
                int i = 0;
                while (*p && *p != '\n' && i < (int)sizeof(line) - 1) line[i++] = *p++;
                line[i] = '\0'; if (*p == '\n') ++p;
                texto(fonte_pequena ? fonte_pequena : NULL, line, al_map_rgb(0,0,0), text_x, text_y, ALLEGRO_ALIGN_LEFT);
                text_y += 18;
            }
        } else {
            const char* p = story;
            while (*p && text_y <= text_bottom) {
                // read paragraph until newline
                char para[2048]; int pi = 0;
                while (*p && *p != '\n' && pi < (int)sizeof(para) - 1) para[pi++] = *p++;
                para[pi] = '\0'; if (*p == '\n') ++p;

                const char* wp = para;
                char linebuf[2048]; linebuf[0] = '\0';
                float line_w = 0.0f;
                float space_w = al_get_text_width(use_font, " ");

                while (*wp && text_y <= text_bottom) {
                    // skip spaces
                    while (*wp == ' ') ++wp;
                    if (!*wp) break;
                    // get next word
                    char word[256]; int wi = 0;
                    while (*wp && *wp != ' ' && wi < (int)sizeof(word) - 1) word[wi++] = *wp++;
                    word[wi] = '\0';
                    float w = al_get_text_width(use_font, word);

                    if (linebuf[0] == '\0') {
                        if (w <= (float)text_w) {
                            snprintf(linebuf, sizeof(linebuf), "%s", word);
                            line_w = w;
                        } else {
                            // split long word into parts that fit
                            char part[256]; int pi2 = 0;
                            for (int k = 0; word[k] != '\0' && text_y <= text_bottom; ++k) {
                                part[pi2++] = word[k]; part[pi2] = '\0';
                                float pw = al_get_text_width(use_font, part);
                                if (pw > (float)text_w) {
                                    // print part without last char
                                    part[pi2-1] = '\0';
                                    texto(use_font, part, al_map_rgb(0,0,0), text_x, text_y, ALLEGRO_ALIGN_LEFT);
                                    text_y += al_get_font_line_height(use_font);
                                    // start new part with current char
                                    part[0] = word[k]; part[1] = '\0'; pi2 = 1;
                                }
                            }
                            if (pi2 > 0 && text_y <= text_bottom) {
                                texto(use_font, part, al_map_rgb(0,0,0), text_x, text_y, ALLEGRO_ALIGN_LEFT);
                                text_y += al_get_font_line_height(use_font);
                            }
                            linebuf[0] = '\0'; line_w = 0.0f;
                        }
                    } else {
                        if (line_w + space_w + w <= (float)text_w) {
                            int len = (int)strlen(linebuf);
                            snprintf(linebuf + len, sizeof(linebuf) - len, " %s", word);
                            line_w += space_w + w;
                        } else {
                            // render current line and start new
                            texto(use_font, linebuf, al_map_rgb(0,0,0), text_x, text_y, ALLEGRO_ALIGN_LEFT);
                            text_y += al_get_font_line_height(use_font);
                            if (text_y > text_bottom) break;
                            if (w <= (float)text_w) {
                                snprintf(linebuf, sizeof(linebuf), "%s", word);
                                line_w = w;
                            } else {
                                // long word at line start
                                char part[256]; int pi2 = 0;
                                for (int k = 0; word[k] != '\0' && text_y <= text_bottom; ++k) {
                                    part[pi2++] = word[k]; part[pi2] = '\0';
                                    float pw = al_get_text_width(use_font, part);
                                    if (pw > (float)text_w) {
                                        part[pi2-1] = '\0';
                                        texto(use_font, part, al_map_rgb(0,0,0), text_x, text_y, ALLEGRO_ALIGN_LEFT);
                                        text_y += al_get_font_line_height(use_font);
                                        part[0] = word[k]; part[1] = '\0'; pi2 = 1;
                                    }
                                }
                                if (pi2 > 0 && text_y <= text_bottom) {
                                    snprintf(linebuf, sizeof(linebuf), "%s", part);
                                    line_w = al_get_text_width(use_font, linebuf);
                                } else { linebuf[0] = '\0'; line_w = 0.0f; }
                            }
                        }
                    }
                }

                if (linebuf[0] != '\0' && text_y <= text_bottom) {
                    texto(use_font, linebuf, al_map_rgb(0,0,0), text_x, text_y, ALLEGRO_ALIGN_LEFT);
                    text_y += al_get_font_line_height(use_font);
                }

                // small paragraph gap
                if (text_y <= text_bottom) text_y += al_get_font_line_height(use_font) / 2;
            }
        }

        if (modal_font) al_destroy_font(modal_font);
    }
}
