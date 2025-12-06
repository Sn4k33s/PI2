#ifndef BOLSAS_H
#define BOLSAS_H

// Estrutura de uma bolsa
typedef struct Bolsa {
    char* nome;
    float preco;
    float last_preco; // preço do dia anterior (para mostrar variação)
    int risco; // 0=baixo, 1=médio, 2=alto
} Bolsa;

// Inicializa internamente (opcional)
void bolsas_init(void);

// Adiciona uma bolsa. Retorna índice (>=0) ou -1 em erro.
int bolsas_add(const char* nome, float preco, int risco);

// Retorna quantidade atual de bolsas
int bolsas_count(void);

// Retorna ponteiro para a bolsa (NULL se índice inválido)
const Bolsa* bolsas_get(int index);

// Remove todas as bolsas e libera memória
void bolsas_clear(void);

// Procura o preço atual de uma bolsa pelo nome. Retorna -1 se não encontrada.
float bolsas_get_price_by_name(const char* name);

// Avança um dia no mercado e atualiza os preços das bolsas
void bolsas_next_day(void);

// Encontra índice da bolsa pelo nome (retorna -1 se não encontrada)
int bolsas_find_by_name(const char* name);

// Define preço de uma bolsa (atualiza last_preco internamente)
void bolsas_set_price(int index, float price);

// Global maximum allowed price for a bolsa (can be used by other modules)
#define BOLSA_PRICE_MAX 1000.0f

#endif // BOLSAS_H