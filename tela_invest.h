#ifndef TELA_INVEST_H
#define TELA_INVEST_H

#ifdef __cplusplus
extern "C" {
#endif

void tela_invest_init(void);
void tela_invest_new(void);
void tela_invest_destroy(void);
void tela_invest_tick(void);
void desenhar_tela_invest(void);
void tela_invest_toggle_noticias(void);
int tela_invest_is_showing_noticias(void);
int tela_invest_get_day(void);
void tela_invest_next_day(void);

// Story modal API: used to show the detailed game story after New Game
void tela_invest_open_story_modal(const char* texto);
int tela_invest_is_story_modal_open(void);
// Handle a mouse click for the modal; returns 1 if the click was consumed (modal closed or clicked), 0 otherwise
int tela_invest_handle_modal_click(int mx, int my);
// Getter for the story text
const char* tela_invest_get_story_text(void);

//
// Mother health API
float tela_invest_get_mae_saude(void);
void tela_invest_adjust_mae(float delta); // accumulates for next day
void tela_invest_apply_treatment(float delta); // immediate effect

// New helpers
int tela_invest_is_finished(void); // non-zero if game ended (mother dead or reached max days)
int tela_invest_get_max_days(void);

// New: query if the mother died (specific game over reason)
int tela_invest_is_mother_dead(void);

// Request the main loop to return to menu (set by invest screen UI/button)
void tela_invest_request_return_to_menu(void);
// Consume and clear pending return-to-menu request; returns 1 if request was pending
int tela_invest_consume_return_request(void);

#ifdef __cplusplus
}
#endif

#endif