#ifndef PLAYER_H
#define PLAYER_H

// Simple player balance and history
typedef struct PlayerEntry {
    char* desc;
    float amount; // positive for income, negative for expense
} PlayerEntry;

typedef struct PlayerHolding {
    char* name;
    float price; // average price paid
    int qty;
} PlayerHolding;

void player_init(void);
void player_destroy(void);

float player_get_balance(void);
void player_set_balance(float b);

void player_add_income(float amount, const char* desc);
void player_add_expense(float amount, const char* desc);

int player_history_count(void);
const PlayerEntry* player_history_get(int index);

// holdings
int player_buy_stock(const char* name, float price, int qty); // returns 0 on success, -1 on fail
int player_sell_stock(const char* name, float price, int qty); // returns qty sold or -1 on fail
int player_portfolio_count(void);
const PlayerHolding* player_portfolio_get(int index);

// save/load to text file (returns 0 on success)
int player_save(const char* filename);
int player_load(const char* filename);

#endif // PLAYER_H
