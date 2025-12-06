#include "bolsas.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

static Bolsa* g_bolsas = NULL;
static int g_count = 0;
static int g_capacity = 0;

void bolsas_init(void)
{
    if (g_capacity == 0) {
        g_capacity = 8;
        g_bolsas = (Bolsa*)malloc(g_capacity * sizeof(Bolsa));
        if (!g_bolsas) g_capacity = 0;
        srand((unsigned)time(NULL));
    }
}

int bolsas_add(const char* nome, float preco, int risco)
{
    if (!nome) return -1;
    if (g_capacity == 0) bolsas_init();
    if (!g_bolsas) return -1;

    if (g_count >= g_capacity) {
        int new_cap = g_capacity * 2;
        Bolsa* tmp = (Bolsa*)realloc(g_bolsas, new_cap * sizeof(Bolsa));
        if (!tmp) return -1;
        g_bolsas = tmp;
        g_capacity = new_cap;
    }

    size_t len = strlen(nome) + 1;
    char* copy = (char*)malloc(len);
    if (!copy) return -1;
    memcpy(copy, nome, len);

    g_bolsas[g_count].nome = copy;
    g_bolsas[g_count].preco = preco;
    g_bolsas[g_count].last_preco = preco;
    g_bolsas[g_count].risco = risco;

    return g_count++;
}

int bolsas_count(void)
{
    return g_count;
}

const Bolsa* bolsas_get(int index)
{
    if (index < 0 || index >= g_count) return NULL;
    return &g_bolsas[index];
}

void bolsas_clear(void)
{
    if (!g_bolsas) return;
    for (int i = 0; i < g_count; ++i) free(g_bolsas[i].nome);
    free(g_bolsas);
    g_bolsas = NULL;
    g_count = 0;
    g_capacity = 0;
}

float bolsas_get_price_by_name(const char* name)
{
    if (!name || !g_bolsas) return -1.0f;
    for (int i = 0; i < g_count; ++i) {
        if (g_bolsas[i].nome && strcmp(g_bolsas[i].nome, name) == 0) return g_bolsas[i].preco;
    }
    return -1.0f;
}

int bolsas_find_by_name(const char* name)
{
    if (!name || !g_bolsas) return -1;
    for (int i = 0; i < g_count; ++i) {
        if (g_bolsas[i].nome && strcmp(g_bolsas[i].nome, name) == 0) return i;
    }
    return -1;
}

void bolsas_set_price(int index, float price)
{
    if (!g_bolsas) return;
    if (index < 0 || index >= g_count) return;
    if (price < 0.01f) price = 0.01f;
    if (price > BOLSA_PRICE_MAX) price = BOLSA_PRICE_MAX;
    g_bolsas[index].last_preco = g_bolsas[index].preco;
    g_bolsas[index].preco = price;
}

// Simula variacao de preco ao avancar um dia
void bolsas_next_day(void)
{
    if (!g_bolsas) return;
    for (int i = 0; i < g_count; ++i) {
        g_bolsas[i].last_preco = g_bolsas[i].preco;
        float base = g_bolsas[i].preco;
        if (base < 0.01f) base = 0.01f;

        // Chance of a large jump/crash independent of news — scaled by risk
        // base chances: low=4%, med=10%, high=16%; tiny extra if very cheap
        float jump_chance = 0.04f + (g_bolsas[i].risco * 0.06f);
        if (base <= 5.0f) jump_chance += 0.05f;

        float rjump = (float)rand() / (float)RAND_MAX;
        if (rjump < jump_chance) {
            // decide boom or crash
            float boom_p = 0.6f - (g_bolsas[i].risco * 0.1f); // higher risk slightly more likely to crash
            if (boom_p < 0.25f) boom_p = 0.25f;
            float r = (float)rand() / (float)RAND_MAX;
            if (r < boom_p) {
                // Big boom: either fixed meteoric targets (100 or 200) or a large multiplier
                float pick = (float)rand() / (float)RAND_MAX;
                float newp;
                if (pick < 0.45f) {
                    // 45% choose 100 or 200
                    newp = ((rand() & 1) == 0) ? 100.0f : 200.0f;
                } else {
                    // otherwise big multiplier 5x..70x
                    float mult = 5.0f + ((float)rand() / (float)RAND_MAX) * 65.0f; // 5..70
                    newp = base * mult;
                }
                if (newp > BOLSA_PRICE_MAX) newp = BOLSA_PRICE_MAX;
                g_bolsas[i].preco = newp;
                continue;
            } else {
                // Big crash: reduce to 1%..20% of value
                float survive = 0.01f + ((float)rand() / (float)RAND_MAX) * 0.19f; // 0.01..0.20
                float np = base * survive;
                if (np < 0.01f) np = 0.01f;
                g_bolsas[i].preco = np;
                continue;
            }
        }

        // Normal daily fluctuation but amplified for faster moves
        float amplitude = 0.02f; // 2% base
        if (g_bolsas[i].risco == 1) amplitude = 0.05f; // 5%
        if (g_bolsas[i].risco == 2) amplitude = 0.12f; // 12%

        // amplify randomly so normal days can also present big swings
        float scale = 1.0f + ((float)rand() / (float)RAND_MAX) * 4.0f; // 1..5
        amplitude *= scale;

        // random factor in [-amplitude, +amplitude]
        float rr = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
        float change = base * amplitude * rr;

        float novo = base + change;
        if (novo < 0.01f) novo = 0.01f; // floor
        if (novo > BOLSA_PRICE_MAX) novo = BOLSA_PRICE_MAX;
        g_bolsas[i].preco = novo;
    }
}