#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "player.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#endif

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

// --- Variáveis internas (mais simples e comentadas em português) ---
// Saldo do jogador
static float saldo = 0.0f;
// Histórico de transações (array dinâmico)
static PlayerEntry* historico = NULL;
static int historico_qtd = 0;
static int historico_cap = 0;
// Posições / holdings do jogador
static PlayerHolding* posicoes = NULL;
static int posicoes_qtd = 0;
static int posicoes_cap = 0;

// Resolve caminho do ficheiro de save para a pasta do executável (Windows) ou usa o nome direto
static const char* resolver_caminho_save(const char* nome)
{
    static char full[MAX_PATH + 1] = {0};
    const char* fn = (nome && nome[0]) ? nome : "player_info.txt";

    // Se já resolvido e for o mesmo nome, retorna
    if (full[0] != '\0') {
        const char* tail = strrchr(full, '\\');
        if (!tail) tail = strrchr(full, '/');
        if (tail && strcmp(tail + 1, fn) == 0) return full;
    }

    // Se o nome já tem caminho, usa direto
    if (strchr(fn, '/') || strchr(fn, '\\')) {
        strncpy(full, fn, sizeof(full)-1);
        full[sizeof(full)-1] = '\0';
        return full;
    }

#ifdef _WIN32
    char mod[MAX_PATH + 1] = {0};
    if (GetModuleFileNameA(NULL, mod, MAX_PATH) > 0) {
        char* p = strrchr(mod, '\\');
        if (!p) p = strrchr(mod, '/');
        if (p) *p = '\0';
        snprintf(full, sizeof(full), "%s\\%s", mod, fn);
        return full;
    }
#endif
    // fallback: nome simples no cwd
    strncpy(full, fn, sizeof(full)-1);
    full[sizeof(full)-1] = '\0';
    return full;
}

// --- Funções internas de gestão de capacidade ---
static int garantir_cap_historico(void) {
    if (historico_qtd < historico_cap) return 1;
    int nc = (historico_cap > 0) ? historico_cap * 2 : 8;
    PlayerEntry* tmp = (PlayerEntry*)realloc(historico, sizeof(PlayerEntry) * nc);
    if (!tmp) return 0;
    historico = tmp;
    historico_cap = nc;
    return 1;
}
static int garantir_cap_posicoes(void) {
    if (posicoes_qtd < posicoes_cap) return 1;
    int nc = (posicoes_cap > 0) ? posicoes_cap * 2 : 8;
    PlayerHolding* tmp = (PlayerHolding*)realloc(posicoes, sizeof(PlayerHolding) * nc);
    if (!tmp) return 0;
    posicoes = tmp;
    posicoes_cap = nc;
    return 1;
}

// --- API pública (mantida) ---
// Inicializa o jogador (saldo inicial padrão)
void player_init(void) {
    saldo = 300.0f; // saldo inicial (reduzido para R$300)
    historico_qtd = 0; historico_cap = 8;
    historico = (PlayerEntry*)malloc(sizeof(PlayerEntry) * historico_cap);
    posicoes_qtd = 0; posicoes_cap = 8;
    posicoes = (PlayerHolding*)malloc(sizeof(PlayerHolding) * posicoes_cap);
}

// Liberta memória usada pelo jogador
void player_destroy(void) {
    if (historico) {
        for (int i = 0; i < historico_qtd; ++i) free(historico[i].desc);
        free(historico);
        historico = NULL;
    }
    historico_qtd = 0; historico_cap = 0;

    if (posicoes) {
        for (int i = 0; i < posicoes_qtd; ++i) free(posicoes[i].name);
        free(posicoes);
        posicoes = NULL;
    }
    posicoes_qtd = 0; posicoes_cap = 0;
}

float player_get_balance(void) { return saldo; }
void player_set_balance(float b) { saldo = b; }

// Adiciona entrada de receita ao histórico e grava
void player_add_income(float amount, const char* desc) {
    if (!garantir_cap_historico()) return;
    size_t l = strlen(desc) + 1;
    char* c = (char*)malloc(l);
    memcpy(c, desc, l);
    historico[historico_qtd].desc = c;
    historico[historico_qtd].amount = amount;
    historico_qtd++;
    saldo += amount;
    player_save(NULL);
}
// Adiciona entrada de despesa ao histórico e grava
void player_add_expense(float amount, const char* desc) {
    if (!garantir_cap_historico()) return;
    size_t l = strlen(desc) + 1;
    char* c = (char*)malloc(l);
    memcpy(c, desc, l);
    historico[historico_qtd].desc = c;
    historico[historico_qtd].amount = -amount;
    historico_qtd++;
    saldo -= amount;
    player_save(NULL);
}

int player_history_count(void) { return historico_qtd; }
const PlayerEntry* player_history_get(int index) {
    if (index < 0 || index >= historico_qtd) return NULL;
    return &historico[index];
}

// Busca índice da posição pelo nome, retorna -1 se não existir
static int achar_posicao(const char* name) {
    if (!name) return -1;
    for (int i = 0; i < posicoes_qtd; ++i) {
        if (posicoes[i].name && strcmp(posicoes[i].name, name) == 0) return i;
    }
    return -1;
}

// Comprar ações: atualiza posições, histórico e saldo, grava
int player_buy_stock(const char* name, float price, int qty) {
    if (!name || qty <= 0) return -1;
    float total = price * qty;
    if (saldo < total) return -1; // sem dinheiro
    if (!garantir_cap_posicoes()) return -1;
    int idx = achar_posicao(name);
    if (idx >= 0) {
        int oldq = posicoes[idx].qty;
        float oldp = posicoes[idx].price;
        int newq = oldq + qty;
        float newp = ((oldp * oldq) + (price * qty)) / (float)newq;
        posicoes[idx].qty = newq;
        posicoes[idx].price = newp;
    } else {
        size_t l = strlen(name) + 1;
        char* c = (char*)malloc(l);
        memcpy(c, name, l);
        posicoes[posicoes_qtd].name = c;
        posicoes[posicoes_qtd].price = price;
        posicoes[posicoes_qtd].qty = qty;
        posicoes_qtd++;
    }
    // histórico
    if (!garantir_cap_historico()) return -1;
    char desc[256];
    snprintf(desc, sizeof(desc), "Compra: %s x%d @ R$ %.2f", name, qty, price);
    size_t dl = strlen(desc) + 1;
    char* dc = (char*)malloc(dl);
    memcpy(dc, desc, dl);
    historico[historico_qtd].desc = dc;
    historico[historico_qtd].amount = -total;
    historico_qtd++;
    saldo -= total;
    player_save(NULL);
    return 0;
}

// Vender ações: atualiza posições, histórico e saldo, grava
int player_sell_stock(const char* name, float price, int qty) {
    if (!name || qty <= 0) return -1;
    int idx = achar_posicao(name);
    if (idx < 0) return -1;
    if (posicoes[idx].qty < qty) return -1;
    float total = price * qty;
    posicoes[idx].qty -= qty;
    if (posicoes[idx].qty == 0) {
        free(posicoes[idx].name);
        for (int j = idx; j < posicoes_qtd - 1; ++j) posicoes[j] = posicoes[j+1];
        posicoes_qtd--;
    }
    if (!garantir_cap_historico()) return -1;
    char desc[256];
    snprintf(desc, sizeof(desc), "Venda: %s x%d @ R$ %.2f", name, qty, price);
    size_t dl = strlen(desc) + 1;
    char* dc = (char*)malloc(dl);
    memcpy(dc, desc, dl);
    historico[historico_qtd].desc = dc;
    historico[historico_qtd].amount = total;
    historico_qtd++;
    saldo += total;
    player_save(NULL);
    return qty;
}

int player_portfolio_count(void) { return posicoes_qtd; }
const PlayerHolding* player_portfolio_get(int index) {
    if (index < 0 || index >= posicoes_qtd) return NULL;
    return &posicoes[index];
}

// Salva estado em disco. Se filename == NULL usa caminho por defeito (resolver_caminho_save)
int player_save(const char* filename) {
    const char* path = resolver_caminho_save(filename);
    FILE* f = fopen(path, "w");
    if (!f) return -1;
    // SALDO
    fprintf(f, "BALANCE %f\n", saldo);
    // POSICOES
    fprintf(f, "HOLDINGS %d\n", posicoes_qtd);
    for (int i = 0; i < posicoes_qtd; ++i) {
        fprintf(f, "%f\t%d\t%s\n", posicoes[i].price, posicoes[i].qty, posicoes[i].name ? posicoes[i].name : "");
    }
    // HISTORICO
    fprintf(f, "HISTORY %d\n", historico_qtd);
    for (int i = 0; i < historico_qtd; ++i) {
        fprintf(f, "%f\t%s\n", historico[i].amount, historico[i].desc ? historico[i].desc : "");
    }
    fflush(f);
    fclose(f);
    return 0;
}

// Carrega estado do disco. Se filename == NULL usa caminho por defeito
int player_load(const char* filename) {
    const char* path = resolver_caminho_save(filename);
    FILE* f = fopen(path, "r");
    if (!f) return -1;
    // limpa existentes
    for (int i = 0; i < historico_qtd; ++i) free(historico[i].desc);
    historico_qtd = 0;
    for (int i = 0; i < posicoes_qtd; ++i) free(posicoes[i].name);
    posicoes_qtd = 0;
    if (!historico) { historico_cap = 8; historico = (PlayerEntry*)malloc(sizeof(PlayerEntry) * historico_cap); if (!historico) { fclose(f); return -1; } }
    if (!posicoes) { posicoes_cap = 8; posicoes = (PlayerHolding*)malloc(sizeof(PlayerHolding) * posicoes_cap); if (!posicoes) { fclose(f); return -1; } }

    char line[512];
    if (fgets(line, sizeof(line), f)) {
        const char* prefix = "BALANCE ";
        size_t pref_len = strlen(prefix);
        if (strncmp(line, prefix, pref_len) == 0) {
            char* endptr = NULL;
            saldo = strtof(line + pref_len, &endptr);
        } else {
            saldo = 0.0f;
        }
    }
    // POSICOES
    if (!fgets(line, sizeof(line), f)) { fclose(f); return 0; }
    int hold_count = 0;
    if (sscanf(line, "HOLDINGS %d", &hold_count) == 1) {
        if (hold_count < 0) hold_count = 0;
        if (hold_count > 1000) hold_count = 1000;
        for (int i = 0; i < hold_count; ++i) {
            if (!fgets(line, sizeof(line), f)) break;
            float price; int qty;
            char name[256] = {0};
            int n = sscanf(line, "%f\t%d\t%255[^\n]", &price, &qty, name);
            if (n != 3) continue;
            if (!garantir_cap_posicoes()) break;
            size_t l = strlen(name) + 1;
            char* c = (char*)malloc(l);
            if (!c) break;
            memcpy(c, name, l);
            posicoes[posicoes_qtd].name = c;
            posicoes[posicoes_qtd].price = price;
            posicoes[posicoes_qtd].qty = qty;
            posicoes_qtd++;
        }
    }
    // HISTORICO
    if (!fgets(line, sizeof(line), f)) { fclose(f); return 0; }
    int hist_count = 0;
    if (sscanf(line, "HISTORY %d", &hist_count) == 1) {
        if (hist_count < 0) hist_count = 0;
        if (hist_count > 10000) hist_count = 10000;
        for (int i = 0; i < hist_count; ++i) {
            if (!fgets(line, sizeof(line), f)) break;
            char* tab = strchr(line, '\t');
            if (!tab) continue;
            *tab = '\0';
            char* amt_str = line;
            char* desc = tab + 1;
            size_t len = strlen(desc);
            while (len > 0 && (desc[len-1] == '\n' || desc[len-1] == '\r')) { desc[len-1] = '\0'; len--; }
            float amt = (float)strtof(amt_str, NULL);
            if (!garantir_cap_historico()) break;
            size_t l = strlen(desc) + 1;
            char* c = (char*)malloc(l);
            if (!c) break;
            memcpy(c, desc, l);
            historico[historico_qtd].desc = c;
            historico[historico_qtd].amount = amt;
            historico_qtd++;
        }
    }
    fclose(f);
    return 0;
}
