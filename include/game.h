#pragma once

#define GAME_MODE_COUNT 4

#include <stdint.h>
#include "input.h"

typedef struct {
    const char * mode_name;

    GameDirection *pattern;
    int pattern_size;
} GameMode;

extern int selected_mode;
extern GameMode gamemodes[GAME_MODE_COUNT];

typedef enum {
    NONE = 0,
    SUCCESS,
    FAIL
} InputAccuracy;

// How many past inputs the history panel remembers
#define INPUT_HISTORY_SIZE 16
// Frame counts stop going up past this, so the number stays short
#define INPUT_HISTORY_MAX_FRAMES 999

// One entry in the input history: a direction and how many frames it was held
typedef struct {
    GameDirection direction;
    int frames;
} InputHistoryEntry;

typedef struct {
    // Ring buffer, newest entry is at `head`
    InputHistoryEntry entries[INPUT_HISTORY_SIZE];
    int head;
    int count;
} InputHistory;

typedef struct{
    // current position in the pattern
    int player_pos;
    
    uint64_t score;
    uint64_t highscore;
    
    GameDirection curr_input;

    InputAccuracy last_input_acc;
    GameDirection last_input;

    InputHistory input_history;
    
    GameMode *current_mode;
    bool run_game;
} GameState;

extern GameState gamestate;

void InitGame();
void DestroyGame();
void _startGame();

void Update(ControllerState *);

void _updateMenu(ControllerState *);
void _updateGame(ControllerState *);

void _initGameModes();
void _resetInputHistory();
void _recordInput(GameDirection);
// Returns the entry `age` inputs ago (0 = newest), or NULL if there isn't one
const InputHistoryEntry *GetInputHistoryEntry(int age);