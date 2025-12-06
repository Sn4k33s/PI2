#ifndef NOTICIAS_H
#define NOTICIAS_H

#include "bolsas.h"

typedef struct Noticia {
    int id;
    char* titulo;
    char* texto;
    char* imagem;    // caminho para imagem (pode ficar vazio)
    int bolsa_index; // -1 se nao linkada
    int setor; // setor afetado (-1 se nao for um grupo)
    float impacto; // multiplicador percentual (-0.2 = -20%, 0.1 = +10%)
    int severity; // 0 = normal, 1 = heavy
} Noticia;

void noticias_init(void);
void noticias_destroy(void);
int noticias_count(void);
const Noticia* noticias_get(int idx);

// Escolhe N noticias para um dado dia (pseudo-aleatorio deterministico)
void noticias_pick_for_day(int day, int* out_indices, int n);

// Aplica uma lista de noticias já selecionadas (altera preços das bolsas)
void noticias_apply_selected(int* indices, int n);

// Função que aplica noticias para o dia (compatibilidade)
void noticias_apply_for_day(int day);

#endif
