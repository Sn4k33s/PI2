#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>
#include "telas.h"
#include "formas.h"
#include "textos.h"
#include "bolsas.h"
#include "player.h"
#include "noticias.h"
#include <stdio.h>
#include <string.h>

void ganhou() {
    // check goal
    if (player_get_balance() >= 5000.0f || dia_atual >= 50) {
        if (dia_atual >= 50 && player_get_balance() <= 5000.0f) {

            snprintf(popup_text, sizeof(popup_text), "Infelizmente voce falhou e nao juntou o valor necessario");
            popup_visivel = 1; popup_timer = 20;

        }
}