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
#include "tela_invest.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

// flag de inicializacao da tela de investimento
static int tela_invest_inited = 0;

// Estado de jogo simples: dia e saude da mae
static int current_day = 1;
static int max_days = 30; // now 30 days limit
static float mae_saude = 100.0f; // por default
static float mae_tratamento_acumulado = 0.0f;

// game state
#define GAME_STATE_PLAYING 0
#define GAME_STATE_MOTHER_DEAD 1
#define GAME_STATE_FINISHED_NO_MONEY 2
#define GAME_STATE_FINISHED_SUCCESS 3
static int game_state = GAME_STATE_PLAYING;

// story modal state
static int story_modal_open = 0;
static char* story_text = NULL;

static const char* INVEST_STATE_FILE = "invest_state.txt";

// parameters for extreme news effects
#define NEWS_BOOM_MIN 10.0f
#define NEWS_BOOM_MAX 1000.0f
#define NEWS_MAX_PRICE 1000.0f

// Request flag used to notify main loop to switch back to menu
static int g_request_return_to_menu = 0;

// Helper to initialize common assets (bolsas list and fonts are handled in callers)
static void tela_invest_common_init(void)
{
    // Ensure bolsas initialized
    bolsas_init();
    noticias_init();
    // Initialize fonts for this screen
    tela_invest_fonts_init();
}

// Save/load invest state (day and mae health)
static void tela_invest_save_state(void)
{
    FILE* f = fopen(INVEST_STATE_FILE, "w");
    if (!f) return;
    fprintf(f, "DAY %d\n", current_day);
    fprintf(f, "MAE %f\n", mae_saude);
    fclose(f);
}

static void tela_invest_load_state(void)
{
    FILE* f = fopen(INVEST_STATE_FILE, "r");
    if (!f) return;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "DAY ", 4) == 0) {
            int d = atoi(line + 4);
            if (d > 0 && d <= max_days) current_day = d;
        } else if (strncmp(line, "MAE ", 4) == 0) {
            float m = (float)atof(line + 4);
            if (m < 0.0f) m = 0.0f; if (m > 100.0f) m = 100.0f;
            mae_saude = m;
        }
    }
    fclose(f);
}

// Inicializa dados da tela de investimento (bolsas etc.) - continue/load
void tela_invest_init(void)
{
    if (tela_invest_inited) return;
    bolsas_init();
    noticias_init();
    player_init();
    // load saved data
    player_load("player_info.txt");

    // Add meme-style bolsas and some low-risk assets
    bolsas_add("DOGE",  3.00f, 2);
    bolsas_add("SHIBA", 2.20f, 2);
    bolsas_add("PEPE", 1.50f, 2);
    bolsas_add("MOON", 5.00f, 1);
    bolsas_add("BANANA", 4.20f, 1);
    bolsas_add("BONGO", 2.80f, 1);
    // low-risk traditional assets
    bolsas_add("ACME", 15.00f, 0);
    bolsas_add("SAFE", 12.50f, 0);
    bolsas_add("UTIL", 8.00f, 0);
    bolsas_add("BLUE", 30.00f, 0);

    tela_invest_fonts_init();

    // load or init
    current_day = 1;
    max_days = 30; // enforce 30 days
    mae_saude = 40.0f; // mae comecar fragil
    mae_tratamento_acumulado = 0.0f;
    game_state = GAME_STATE_PLAYING;

    // load persisted invest state (overrides defaults)
    tela_invest_load_state();

    tela_invest_inited = 1;
}

// Start a new game (reset player and start with R$500)
void tela_invest_new(void)
{
    // If already initialized, clean up previous state without saving
    if (tela_invest_inited) {
        // destroy without saving
        bolsas_clear();
        player_destroy();
        tela_invest_fonts_destroy();
        noticias_destroy();
        tela_invest_inited = 0;
    }

    bolsas_init();
    noticias_init();
    player_init();
    player_set_balance(500.0f); // new game balance

    // add meme bolsas and low-risk assets
    bolsas_add("DOGE",  3.00f, 2);
    bolsas_add("SHIBA", 2.20f, 2);
    bolsas_add("PEPE", 1.50f, 2);
    bolsas_add("MOON", 5.00f, 1);
    bolsas_add("BANANA", 4.20f, 1);
    bolsas_add("BONGO", 2.80f, 1);
    bolsas_add("ACME", 15.00f, 0);
    bolsas_add("SAFE", 12.50f, 0);
    bolsas_add("UTIL", 8.00f, 0);
    bolsas_add("BLUE", 30.00f, 0);

    // reset simple state
    current_day = 1;
    max_days = 30; // new game uses 30 days
    mae_saude = 40.0f; // mae comecar fragil
    mae_tratamento_acumulado = 0.0f;
    game_state = GAME_STATE_PLAYING;

    tela_invest_fonts_init();
    tela_invest_inited = 1;

    // persist immediately so a later "Continuar" after restart loads this state
    player_save("player_info.txt");
    tela_invest_save_state();

    // open story modal with Win95-like content (ASCII only)
    const char* story = "Historia - Cash Flow 2000:\n\nVoce e responsavel por cuidar da sua mae enquanto tenta pagar uma divida.\nObjetivo: Chegar ate o dia 30 com sua mae viva e juntar R$ 5000 para pagar a divida.\n\nBoas praticas: diversifique entre bolsas diferentes (memes e ativos tradicionais), cuide da saude da sua mae com remedios e visitas. Noticias diarias tentam prever o mercado - mas podem falhar; algumas podem causar altas ou quedas repentinas. Use diversificacao e gestao de risco. Boa sorte!";
    tela_invest_open_story_modal(story);
}

// Limpa recursos da tela de investimento
void tela_invest_destroy(void)
{
    if (!tela_invest_inited) return;
    // salva estado
    player_save("player_info.txt");
    tela_invest_save_state();
    player_destroy();
    bolsas_clear();
    noticias_destroy();
    tela_invest_fonts_destroy();
    tela_invest_inited = 0;
}

// Função de desenho (chamada por atualizar_tela)
void desenhar_tela_invest(void)
{
    tela_invest(140, 180, 1140, 780);
    barra_de_tarefas();
    aba_padrao(10, 924, 100, 955);
}

// --- Simple day/health API used by formas.c --------------------------------
int tela_invest_get_day(void)
{
    return current_day;
}

int tela_invest_get_max_days(void) {
    return max_days;
}

int tela_invest_is_finished(void) {
    return (game_state != GAME_STATE_PLAYING);
}

// New: query whether the mother died (specific game over)
int tela_invest_is_mother_dead(void) {
    return (game_state == GAME_STATE_MOTHER_DEAD);
}

// Request/consume return-to-menu
void tela_invest_request_return_to_menu(void) {
    g_request_return_to_menu = 1;
}
int tela_invest_consume_return_request(void) {
    int v = g_request_return_to_menu;
    g_request_return_to_menu = 0;
    return v;
}

void tela_invest_next_day(void)
{
    if (!tela_invest_inited) return;
    if (game_state != GAME_STATE_PLAYING) return; // no further progress after game end

    if (current_day < max_days) current_day++;
    // health decay per day
    mae_saude -= 2.0f; // perde 2% a cada dia
    if (mae_saude < 0.0f) mae_saude = 0.0f;
    // tratamento acumulado ajuda
    if (mae_tratamento_acumulado > 0.0f) {
        mae_saude += mae_tratamento_acumulado;
        if (mae_saude > 100.0f) mae_saude = 100.0f;
    }
    mae_tratamento_acumulado = 0.0f; // reset diario

    // apply daily news effects
    noticias_apply_for_day(current_day);

    // save state after advancing day
    tela_invest_save_state();

    // check end conditions
    if (mae_saude <= 0.0f) {
        mae_saude = 0.0f;
        game_state = GAME_STATE_MOTHER_DEAD;
        // Do not open modal here; UI will show Game Over overlay and return button
        return;
    }

    if (current_day >= max_days) {
        // reached final day
        float bal = player_get_balance();
        if (bal >= 5000.0f) {
            game_state = GAME_STATE_FINISHED_SUCCESS;
            tela_invest_open_story_modal("Parabens! Voce completou o jogo e ja esta pronto para investir.");
        } else {
            game_state = GAME_STATE_FINISHED_NO_MONEY;
            tela_invest_open_story_modal("Parabens! Chegou ao final do periodo.");
        }
    }
}

float tela_invest_get_mae_saude(void)
{
    return mae_saude;
}

void tela_invest_adjust_mae(float delta)
{
    mae_tratamento_acumulado += delta;
}

// immediate effect purchase (called when buying treatment)
void tela_invest_apply_treatment(float delta)
{
    if (!tela_invest_inited) {
        // accumulate anyway so next day it helps
        mae_tratamento_acumulado += delta;
        return;
    }
    mae_saude += delta;
    if (mae_saude > 100.0f) mae_saude = 100.0f;
    // save after treatment purchase
    tela_invest_save_state();
}

// Story modal API
void tela_invest_open_story_modal(const char* texto)
{
    if (story_text) { free(story_text); story_text = NULL; }
    if (texto) {
        size_t l = strlen(texto) + 1;
        story_text = (char*)malloc(l);
        memcpy(story_text, texto, l);
        story_modal_open = 1;
    }
}

int tela_invest_is_story_modal_open(void)
{
    return story_modal_open;
}

const char* tela_invest_get_story_text(void)
{
    return story_text ? (const char*)story_text : "";
}

int tela_invest_handle_modal_click(int mx, int my)
{
    if (!story_modal_open) return 0;
    // modal position should match formas.c; compute here
    int win_w = 820; int win_h = 420;
    int win_x = (1280 - win_w)/2;
    int win_y = (960 - win_h)/2;
    // close button at top-right of modal: 24x24 inside window
    int bx = win_x + win_w - 28; int by = win_y + 6; int bw = 20; int bh = 20;
    if ( mx >= bx && mx <= bx + bw && my >= by && my <= by + bh ) {
        story_modal_open = 0;
        return 1;
    }
    // consume clicks inside modal so underlying UI doesn't react
    if ( mx >= win_x && mx <= win_x + win_w && my >= win_y && my <= win_y + win_h ) return 1;
    return 0;
}

// --- Simple embedded news system (to ensure linker has implementations) ---
#include <stdint.h>

typedef struct NoticiaInternal {
    int id;
    char* titulo;
    char* texto;
    char* imagem;
    int bolsa_index;
    int setor;
    float impacto;
} NoticiaInternal;

static NoticiaInternal* g_noticias_int = NULL;
static int g_n_count_int = 0;
static int g_n_cap_int = 0;

static void ensure_noticias_space_int(int n) {
    if (g_n_cap_int >= n) return;
    int nc = g_n_cap_int > 0 ? g_n_cap_int * 2 : 16;
    while (nc < n) nc *= 2;
    NoticiaInternal* tmp = (NoticiaInternal*)realloc(g_noticias_int, sizeof(NoticiaInternal) * nc);
    if (!tmp) return;
    g_noticias_int = tmp;
    g_n_cap_int = nc;
}

void noticias_init(void) {
    if (g_n_count_int > 0) return; // already inited
    ensure_noticias_space_int(16);
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
    float impactos[] = { 0.30f, 0.25f, 0.22f, 0.28f, -0.35f, -0.18f, 0.12f, 0.15f, -0.40f, -0.20f, 0.40f, 0.18f, 0.06f, 0.04f, -0.03f, 0.05f };
    int n = sizeof(titulos) / sizeof(titulos[0]);
    for (int i = 0; i < n; ++i) {
        ensure_noticias_space_int(i + 1);
        g_noticias_int[i].id = i;
        size_t lt = strlen(titulos[i]) + 1;
        g_noticias_int[i].titulo = (char*)malloc(lt); memcpy(g_noticias_int[i].titulo, titulos[i], lt);
        size_t lx = strlen(textos[i]) + 1;
        g_noticias_int[i].texto = (char*)malloc(lx); memcpy(g_noticias_int[i].texto, textos[i], lx);
        g_noticias_int[i].imagem = (char*)malloc(1); g_noticias_int[i].imagem[0] = '\0';
        g_noticias_int[i].impacto = impactos[i];
        g_noticias_int[i].bolsa_index = -1;
        g_noticias_int[i].setor = -1;
        if (map_names[i] && map_names[i][0] != '\0') {
            int idx = bolsas_find_by_name(map_names[i]);
            if (idx >= 0) g_noticias_int[i].bolsa_index = idx;
        }
    }
    g_n_count_int = n;
}

void noticias_destroy(void) {
    if (!g_noticias_int) return;
    for (int i = 0; i < g_n_count_int; ++i) {
        free(g_noticias_int[i].titulo);
        free(g_noticias_int[i].texto);
        free(g_noticias_int[i].imagem);
    }
    free(g_noticias_int);
    g_noticias_int = NULL; g_n_count_int = 0; g_n_cap_int = 0;
}

int noticias_count(void) { return g_n_count_int; }

// Return pointer compatible with Noticia (same layout) - safe because fields match
const Noticia* noticias_get(int idx) {
    if (idx < 0 || idx >= g_n_count_int) return NULL;
    return (const Noticia*)&g_noticias_int[idx];
}

// Deterministic PRNG for picks
static uint32_t seed_prng_day(uint32_t s) { return s * 1664525u + 1013904223u; }

void noticias_pick_for_day(int day, int* out_indices, int n) {
    if (!g_noticias_int || n <= 0) return;
    for (int i = 0; i < n; ++i) out_indices[i] = -1;
    int picked = 0;
    uint32_t seed = (uint32_t)day + 0x9e3779b9u;
    int attempts = 0;
    while (picked < n && attempts < g_n_count_int * 10) {
        seed = seed_prng_day(seed);
        int idx = (int)(seed % (uint32_t)g_n_count_int);
        int dup = 0;
        for (int j = 0; j < picked; ++j) if (out_indices[j] == idx) { dup = 1; break; }
        if (!dup) { out_indices[picked++] = idx; }
        attempts++;
    }

    int k = 0;
    while (picked < n && k < g_n_count_int) {
        int dup = 0;
        for (int j = 0; j < picked; ++j) if (out_indices[j] == k) { dup = 1; break; }
        if (!dup) out_indices[picked++] = k;
        k++;
    }
}

// Apply selected noticias similar to noticias.c logic
static float prng_unit_local(uint32_t* seed) {
    *seed = (*seed) * 1664525u + 1013904223u;
    return (float)(*seed & 0xFFFFFFu) / (float)0x1000000u;
}

void noticias_apply_selected(int* indices, int n) {
    if (!g_noticias_int || !indices) return;
    for (int i = 0; i < n; ++i) {
        int idx = indices[i];
        if (idx < 0 || idx >= g_n_count_int) continue;
        NoticiaInternal* no = &g_noticias_int[idx];
        if (no->bolsa_index >= 0) {
            const Bolsa* b = bolsas_get(no->bolsa_index);
            if (!b) continue;
            float old = b->preco;
            uint32_t seed = (uint32_t)(idx * 1103515245u + (uint32_t)time(NULL));
            float r1 = prng_unit_local(&seed);
            float r2 = prng_unit_local(&seed);
            float impact = no->impacto;
            float risk_mul = 1.0f + (b->risco * 0.7f);
            float crash_chance = fmaxf(0.02f, fabsf(impact) * 0.25f) * risk_mul;
            float boom_chance = fmaxf(0.02f, fabsf(impact) * 0.20f) * (1.0f + (0.5f - (b->risco * 0.15f)));
            int applied = 0;
            if (impact < 0.0f) {
                if (!applied && r1 < crash_chance) {
                    float survive = 0.01f + prng_unit_local(&seed) * 0.05f;
                    float np = old * survive;
                    if (np < 0.01f) np = 0.01f;
                    bolsas_set_price(no->bolsa_index, np);
                    applied = 1;
                }
            }
            if (!applied) {
                if (r1 < boom_chance) {
                    float mult = NEWS_BOOM_MIN + prng_unit_local(&seed) * (NEWS_BOOM_MAX - NEWS_BOOM_MIN);
                    float np = old * mult;
                    if (np > NEWS_MAX_PRICE) np = NEWS_MAX_PRICE;
                    bolsas_set_price(no->bolsa_index, np);
                    applied = 1;
                }
            }
            if (!applied) {
                float sign = (prng_unit_local(&seed) < 0.8f) ? (impact >= 0.0f ? 1.0f : -1.0f) : (impact >= 0.0f ? -1.0f : 1.0f);
                float mag = fabsf(impact) * (1.0f + prng_unit_local(&seed) * 10.0f);
                float change = old * (sign * mag);
                float np = old + change;
                if (np < 0.01f) np = 0.01f;
                if (np > NEWS_MAX_PRICE) np = NEWS_MAX_PRICE;
                bolsas_set_price(no->bolsa_index, np);
                applied = 1;
            }
        }
    }
}

void noticias_apply_for_day(int day) {
    if (!g_noticias_int) return;
    int count = 1 + (day % 3);
    int picks[4]; for (int i = 0; i < count; ++i) picks[i] = -1;
    noticias_pick_for_day(day, picks, count);
    noticias_apply_selected(picks, count);
}
