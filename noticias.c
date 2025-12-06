#include "noticias.h"
#include "bolsas.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <time.h>

static Noticia* g_noticias = NULL;
static int g_n_count = 0;
static int g_n_cap = 0;

static void ensure_noticias_space(int n) {
    if (g_n_cap >= n) return;
    int nc = g_n_cap > 0 ? g_n_cap * 2 : 16;
    while (nc < n) nc *= 2;
    Noticia* tmp = (Noticia*)realloc(g_noticias, sizeof(Noticia) * nc);
    if (!tmp) return;
    g_noticias = tmp;
    g_n_cap = nc;
}

void noticias_init(void) {
    if (g_n_count > 0) return; // already inited
    ensure_noticias_space(32);

    // Meme-themed news (positive and negative) + some low-risk asset news
    const char* titulos[] = {
        "Doge vira mascote do BANCO",
        "Shiba challenge viraliza",
        "Pepe ganha apoio de influencer",
        "MoonTech anuncia prototipo",
        "BananaCoin sofre hack em exchange",
        "BongoCorp publica memorando duvidoso",
        "Doge faz partnership com pizzaria",
        "Shiba lanca colecao de NFTs",
        "Pepe acusado de fraude",
        "Moon token sofre correcao tecnica",
        "Banana recebe oferta de compra surpresa",
        "Bongo anuncia vendas recorde",
        "ACME reports stable growth",
        "SAFE raises dividend",
        "UTIL schedules maintenance",
        "BLUE upgrades guidance"
    };
    const char* textos[] = {
        "Uma celebridade postou foto com meme, a comunidade reage com entusiasmo.",
        "Video challenge aumenta a demanda por tokens Shiba, traders especulam.",
        "Meme popular recebe mencao em rede social, especulacoes sobem.",
        "Startup ligada ao projeto anuncia avancos; investidores sonham com big upside.",
        "Vulnerabilidade exposta permite saque parcial; negociacoes suspendidas.",
        "Documento interno vaza com linguagem confusa, mercado reage com cautela.",
        "A pizzaria anuncia que aceita DOGE; mercado reage de forma bem-humorada.",
        "Colecao limitada de NFTs ligada ao meme gera hype temporario.",
        "Acusacoes fortes surgem contra custodiante; nervosismo domina.",
        "Problema tecnico forca reinicio de rede; liquidez cai temporariamente.",
        "Oferta misteriosa eleva especulacao; investidores divididos.",
        "Relatorio financeiro mostra vendas acima do esperado; confianca aumenta.",
        "ACME reports consistent quarter-over-quarter growth with steady margins.",
        "SAFE declares a modest dividend increase, appealing to conservative investors.",
        "UTIL announces scheduled maintenance that may temporarily affect supply.",
        "BLUE upgrades guidance after positive client contracts are signed."
    };
    const char* map_names[] = {
        "DOGE", "SHIBA", "PEPE", "MOON", "BANANA", "BONGO",
        "DOGE", "SHIBA", "PEPE", "MOON", "BANANA", "BONGO",
        "ACME", "SAFE", "UTIL", "BLUE"
    };
    // impacto is a baseline magnitude for the news sentiment (can be used as percent)
    float impactos[] = { 0.30f, 0.25f, 0.22f, 0.28f, -0.35f, -0.18f, 0.12f, 0.15f, -0.40f, -0.20f, 0.40f, 0.18f, 0.06f, 0.04f, -0.03f, 0.05f };

    int n = sizeof(titulos) / sizeof(titulos[0]);
    for (int i = 0; i < n; ++i) {
        ensure_noticias_space(i + 1);
        g_noticias[i].id = i;
        size_t lt = strlen(titulos[i]) + 1;
        g_noticias[i].titulo = (char*)malloc(lt); memcpy(g_noticias[i].titulo, titulos[i], lt);
        size_t lx = strlen(textos[i]) + 1;
        g_noticias[i].texto = (char*)malloc(lx); memcpy(g_noticias[i].texto, textos[i], lx);
        g_noticias[i].imagem = (char*)malloc(1); g_noticias[i].imagem[0] = '\0';
        g_noticias[i].impacto = impactos[i];
        g_noticias[i].bolsa_index = -1;
        g_noticias[i].setor = -1;
        if (map_names[i] && map_names[i][0] != '\0') {
            int idx = bolsas_find_by_name(map_names[i]);
            if (idx >= 0) g_noticias[i].bolsa_index = idx;
        }
    }
    g_n_count = n;
}

void noticias_destroy(void) {
    if (!g_noticias) return;
    for (int i = 0; i < g_n_count; ++i) {
        free(g_noticias[i].titulo);
        free(g_noticias[i].texto);
        free(g_noticias[i].imagem);
    }
    free(g_noticias);
    g_noticias = NULL; g_n_count = 0; g_n_cap = 0;
}

int noticias_count(void) { return g_n_count; }
const Noticia* noticias_get(int idx) { if (idx < 0 || idx >= g_n_count) return NULL; return &g_noticias[idx]; }

// deterministic pseudo-random float in [0,1) based on seed
static float prng_unit(uint32_t* seed) {
    *seed = (*seed) * 1664525u + 1013904223u;
    return (float)(*seed & 0xFFFFFFu) / (float)0x1000000u;
}

void noticias_pick_for_day(int day, int* out_indices, int n) {
    if (!g_noticias || n <= 0) return;
    for (int i = 0; i < n; ++i) out_indices[i] = -1;
    int picked = 0;
    uint32_t seed = (uint32_t)day + 0x9e3779b9u;
    int attempts = 0;
    while (picked < n && attempts < g_n_count * 10) {
        // deterministic pick using LCG
        seed = seed * 1664525u + 1013904223u;
        int idx = (int)(seed % (uint32_t)g_n_count);
        int dup = 0;
        for (int j = 0; j < picked; ++j) if (out_indices[j] == idx) { dup = 1; break; }
        if (!dup) { out_indices[picked++] = idx; }
        attempts++;
    }
    int k = 0;
    while (picked < n && k < g_n_count) {
        int dup = 0;
        for (int j = 0; j < picked; ++j) if (out_indices[j] == k) { dup = 1; break; }
        if (!dup) out_indices[picked++] = k;
        k++;
    }
}

// apply selected noticias with dramatic outcomes (boom / crash / normal)
void noticias_apply_selected(int* indices, int n) {
    if (!g_noticias || !indices) return;
    for (int i = 0; i < n; ++i) {
        int idx = indices[i];
        if (idx < 0 || idx >= g_n_count) continue;
        Noticia* no = &g_noticias[idx];
        if (no->bolsa_index >= 0) {
            const Bolsa* b = bolsas_get(no->bolsa_index);
            if (!b) continue;
            float old = b->preco;
            // deterministic-ish randomness based on noticia id and current time
            uint32_t seed = (uint32_t)(idx * 1103515245u + (uint32_t)time(NULL));
            float r1 = prng_unit(&seed); // 0..1
            float r2 = prng_unit(&seed);

            // base factor influenced by noticia impact
            float impact = no->impacto; // may be negative (bad news)

            // compute crash and boom probabilities influenced by risk and impact magnitude
            float risk_mul = 1.0f + (b->risco * 0.7f);
            float crash_chance = fmaxf(0.02f, fabsf(impact) * 0.25f) * risk_mul; // base chance
            float boom_chance = fmaxf(0.02f, fabsf(impact) * 0.20f) * (1.0f + (0.5f - (b->risco * 0.15f)));

            // small chance of total failure for high risk + bad news
            int applied = 0;
            float np = old;

            // Special: if the stock price is very low (<= 4.0), allow a rare meteoric jump to 100 or 200
            if (!applied && old <= 4.0f) {
                // Jump chance scaled by boom_chance and impact magnitude
                float jump_chance = boom_chance * 1.5f + fabsf(impact) * 0.1f; // modest extra weight
                if (jump_chance > 0.6f) jump_chance = 0.6f; // limit
                if (prng_unit(&seed) < jump_chance) {
                    // choose target 100 or 200 (50/50)
                    float pick = prng_unit(&seed);
                    np = (pick < 0.5f) ? 100.0f : 200.0f;
                    if (np > BOLSA_PRICE_MAX) np = BOLSA_PRICE_MAX;
                    applied = 1;
                }
            }

            if (!applied && impact < 0.0f) {
                // bad news increases crash chance
                if (r1 < crash_chance) {
                    float survive = 0.01f + prng_unit(&seed) * 0.05f; // 1%..6% survive
                    np = old * survive;
                    if (np < 0.01f) np = 0.01f;
                    applied = 1;
                }
            }
            if (!applied) {
                // chance of huge boom
                if (r1 < boom_chance) {
                    // boom multiplier – make generally larger but capped by BOLSA_PRICE_MAX
                    float mult = 1.0f + fabsf(impact) * (2.0f + prng_unit(&seed) * 6.0f); // big multiplier
                    np = old * mult;
                    if (np > BOLSA_PRICE_MAX) np = BOLSA_PRICE_MAX;
                    applied = 1;
                }
            }
            if (!applied) {
                // normal application: small to medium change, direction favored by noticia impact
                // decide direction: 80% follow the news sign, 20% opposite
                float sign = (prng_unit(&seed) < 0.8f) ? (impact >= 0.0f ? 1.0f : -1.0f) : (impact >= 0.0f ? -1.0f : 1.0f);
                float mag = fabsf(impact) * (0.5f + prng_unit(&seed) * 1.5f); // 0.5..2.0 * impact
                float change = old * (sign * mag);
                np = old + change;
                if (np < 0.01f) np = 0.01f;
                if (np > BOLSA_PRICE_MAX) np = BOLSA_PRICE_MAX;
                applied = 1;
            }

            if (applied) {
                // apply the first computed price
                bolsas_set_price(no->bolsa_index, np);

                // Now apply probabilistic continuation: if the news has positive impact,
                // each additional successful trial (probability = impact) will increase price further
                // until it reaches BOLSA_PRICE_MAX. For negative impact, it may continue falling with
                // probability = |impact| until floor 0.01f.
                if (impact > 0.0f) {
                    int reps = 0; const int MAX_REPS = 64;
                    while (prng_unit(&seed) < impact && ++reps < MAX_REPS) {
                        // multiply by (1 + impact) for each continuation step
                        np = np * (1.0f + impact);
                        if (np > BOLSA_PRICE_MAX) np = BOLSA_PRICE_MAX;
                        bolsas_set_price(no->bolsa_index, np);
                        if (np >= BOLSA_PRICE_MAX) break;
                    }
                } else if (impact < 0.0f) {
                    int reps = 0; const int MAX_REPS = 64;
                    float prob = fabsf(impact);
                    while (prng_unit(&seed) < prob && ++reps < MAX_REPS) {
                        // reduce by impact fraction each step
                        float reduce = fabsf(impact);
                        np = np * (1.0f - reduce);
                        if (np < 0.01f) np = 0.01f;
                        bolsas_set_price(no->bolsa_index, np);
                        if (np <= 0.01f) break;
                    }
                }
            }
        }
    }
}

void noticias_apply_for_day(int day) {
    if (!g_noticias) return;
    // choose a deterministic number of noticias: 1..3
    int count = 1 + (day % 3);
    int picks[4]; for (int i = 0; i < count; ++i) picks[i] = -1;
    noticias_pick_for_day(day, picks, count);
    noticias_apply_selected(picks, count);
}
