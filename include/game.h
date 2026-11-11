#pragma once

#define GAME_MODE_COUNT 6

#include <stdbool.h>
#include <stdint.h>
#include "input.h"

typedef struct {
    const char * mode_name;

    GameDirection *pattern;
    int pattern_size;

    // Pattern steps to show timing stats for, in display order
    const int *stat_steps;
    int stat_step_count;

    // Random movement string mode: no fixed pattern, moves are generated
    bool random_moves;
    // Swap forward and back (P2 side)
    bool mirrored;
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

// ************* RANDOM MOVEMENT STRING ******************//

typedef enum {
    MOVE_BACK_DASH = 0,
    MOVE_KBD,
    MOVE_SIDE_STEP_UP,
    MOVE_SIDE_WALK_UP,
    MOVE_SIDE_STEP_DOWN,
    MOVE_SIDE_WALK_DOWN,
    MOVE_FORWARD_DASH,
    MOVE_WAVE_DASH,
    // The moves below are never picked on their own:
    // between two forward dashes
    MOVE_BLOCK,
    // A back dash into the first KBD of a chain, shown as one move
    MOVE_BACK_DASH_KBD,
    // A dash into a wave dash, shown as one move
    MOVE_DASH_WAVE_DASH,
    MOVE_TYPE_COUNT
} MoveType;

// How the last input of a move has to be held
typedef enum {
    HOLD_ANY = 0,
    // Released within SIDE_STEP_FRAMES (side step)
    HOLD_TAP
} MoveHold;

// A side step (or a side walk's first tap) has to be let go within this
// many frames
#define SIDE_STEP_FRAMES 15

#define MAX_MOVE_STEPS 7

typedef struct {
    const char *name;
    // Inputs for P1. P2 swaps forward and back.
    GameDirection steps[MAX_MOVE_STEPS];
    int step_count;
    MoveHold hold;
    // The first input has to be let go within SIDE_STEP_FRAMES (side steps
    // and the first tap of side walks)
    bool first_tap;
} MoveInfo;

extern const MoveInfo move_info[MOVE_TYPE_COUNT];

// How many upcoming moves are shown
#define MOVES_SHOWN 5

// One row of the move list: a move done `reps` times in a row. Only KBD
// chains (BACKDASH KBD, then KBDs) and wave dashes repeat.
typedef struct {
    MoveType type;
    int reps;
} MoveRow;

// Most reps one row asks for
#define MAX_ROW_REPS 4

typedef struct {
    // queue[0] is the row being input, the rest are coming up
    MoveRow queue[MOVES_SHOWN];
    // The row that was just finished, for the scroll animation
    MoveRow last_row;
    // Reps of queue[0] done so far
    int rep;
    // Inputs of the current rep done so far
    int step;
    // Moves (reps) finished this run
    int moves_done;
    // Rows finished this run
    int rows_done;
    // Last input of the previous move. Releasing part of it is not a miss.
    GameDirection release_from;
    // Move that has to come next (the dash after a block), or
    // MOVE_TYPE_COUNT for none
    MoveType pending;
} MoveString;

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

    // Random movement string mode
    MoveString moves;
    
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

void _resetMoves();
void _updateMoves(ControllerState *);
// Input `step` of a move, with forward and back swapped on the P2 side
GameDirection GetMoveStep(MoveType move, int step);
// The move for rep `rep` of a row (the first KBD of a chain has its back
// dash in front, later ones don't)
MoveType GetRowMove(const MoveRow *row, int rep);
// The move whose name to show. Side steps and walks are named by the side
// they go to (L/R): up is left on the P1 side and right on the P2 side.
MoveType GetNameMove(MoveType move);
