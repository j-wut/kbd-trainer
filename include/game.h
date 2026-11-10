#pragma once

#define GAME_MODE_COUNT 4

#include <stdint.h>
#include "input.h"

typedef struct {
    const char * mode_name;

    GameDirection *pattern;
    int pattern_size;

    // Pattern steps to show timing stats for, in display order
    const int *stat_steps;
    int stat_step_count;
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
    // True if this input caused a MISS
    bool missed;
} InputHistoryEntry;

typedef struct {
    // Ring buffer, newest entry is at `head`
    InputHistoryEntry entries[INPUT_HISTORY_SIZE];
    int head;
    int count;
} InputHistory;

// Longest pattern any mode uses
#define MAX_PATTERN_SIZE 8
// How many recent reps the timing stats are taken over
#define STEP_STATS_WINDOW 10

// Frames held on one pattern step over the most recent reps
typedef struct {
    // Ring buffer, newest sample is at `head`
    int samples[STEP_STATS_WINDOW];
    int head;
    int count;
} StepStats;

// Summary of a StepStats window, for display
typedef struct {
    int count;
    int last;
    int min;
    int max;
    float avg;
} StepSummary;

typedef struct{
    // current position in the pattern
    int player_pos;
    
    uint64_t score;
    uint64_t highscore;
    
    GameDirection curr_input;

    InputAccuracy last_input_acc;
    GameDirection last_input;

    InputHistory input_history;

    // Direction currently held and for how many frames
    GameDirection held_direction;
    int held_frames;

    // Frames held on each pattern step, indexed by step
    StepStats step_stats[MAX_PATTERN_SIZE];
    
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
void _markLastInputMissed();
// Returns the entry `age` inputs ago (0 = newest), or NULL if there isn't one
const InputHistoryEntry *GetInputHistoryEntry(int age);

void _resetStepStats();
void _recordStepFrames(int step, int frames);
// Summarizes the recent frames held on a pattern step. count is 0 if there are none yet.
StepSummary GetStepSummary(int step);
