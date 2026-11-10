#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "game.h"
#include "input.h"

ControllerState prev_input = {0};
int selected_mode = 0;

GameMode gamemodes[GAME_MODE_COUNT];
GameState gamestate;

uint64_t highscores[GAME_MODE_COUNT] = {0};

void InitGame()
{
    _initGameModes();
    
    GameState initGS = {0};
    initGS.current_mode = gamemodes; 

    gamestate = initGS;
}

void Update(ControllerState *cs)
{
    if (gamestate.run_game)
        _updateGame(cs);
    else
        _updateMenu(cs);
}

void _updateMenu(ControllerState *cs)
{
    if (cs->back_pressed == prev_input.back_pressed
        && cs->select_pressed == prev_input.select_pressed
        && cs->direction == prev_input.direction)
        return;

    prev_input = *cs;
    
    // Start game
    if (cs->select_pressed)
    {
        _startGame();
        return;
    }

    if (cs->direction == FORWARD)
    {
        if (selected_mode == GAME_MODE_COUNT - 1)
        selected_mode = 0;
        else
        selected_mode += 1;
    }
    else if (cs->direction == BACK)
    {
        if (selected_mode == 0)
        selected_mode = GAME_MODE_COUNT - 1;
        else
        selected_mode -= 1;
    }
    
    gamestate.current_mode = &gamemodes[selected_mode];
}

void _updateGame(ControllerState *cs)
{
    gamestate.curr_input = cs->direction;
    _recordInput(cs->direction);

    // Count how long the current direction has been held. When it changes,
    // ended_frames is how long the previous direction was held.
    int ended_frames = 0;
    if (cs->direction == gamestate.held_direction)
    {
        if (gamestate.held_frames < INPUT_HISTORY_MAX_FRAMES)
            gamestate.held_frames += 1;
    }
    else
    {
        ended_frames = gamestate.held_frames;
        gamestate.held_direction = cs->direction;
        gamestate.held_frames = 1;
    }

    if (gamestate.last_input_acc == FAIL)
    {
        // Reset after failure
        gamestate.player_pos = 0;
        gamestate.score = 0;
        gamestate.last_input_acc = NONE;
        prev_input = *cs;
        return;
    }

    if (cs->back_pressed)
    {
        highscores[selected_mode] = gamestate.highscore;
        gamestate.run_game = false;
        return;
    }
    
    // No update if input has not changed
    if (cs->direction == prev_input.direction) return;
    
    if (cs->direction == gamestate.current_mode->pattern[ gamestate.player_pos % gamestate.current_mode->pattern_size ])
    {
        // Correct input. If the previous step was also hit correctly in this
        // run, the direction that just ended was that step, so record its timing.
        if (gamestate.player_pos != 0 || gamestate.score > 0)
        {
            int size = gamestate.current_mode->pattern_size;
            _recordStepFrames((gamestate.player_pos + size - 1) % size, ended_frames);
        }

        gamestate.score += 50;
        if (gamestate.score > gamestate.highscore)
            gamestate.highscore = gamestate.score;
        
        if (gamestate.player_pos == gamestate.current_mode->pattern_size - 1)
            gamestate.player_pos = 0;
        else
            gamestate.player_pos += 1;
        
        prev_input.direction = cs->direction;
        
        gamestate.last_input = cs->direction;
        gamestate.last_input_acc = SUCCESS;
    }
    else
    {
        // Incorrect input
        prev_input.direction = cs->direction;

        // Wrong inputs are only ignored before the first correct input of a run.
        // Once the pattern loops back to the start, they count as misses.
        if (gamestate.player_pos != 0 || gamestate.score > 0)
        {
            gamestate.last_input = cs->direction;
            gamestate.last_input_acc = FAIL;
            _markLastInputMissed();
        }
    }
}

void _startGame()
{
    printf("Mode(%d) score: %" PRIu64 "\n", selected_mode, highscores[selected_mode]);
    gamestate.player_pos = 0;
    gamestate.score = 0;
    gamestate.highscore = highscores[selected_mode];

    gamestate.last_input = NEUTRAL;
    gamestate.last_input_acc = NONE;
    _resetInputHistory();
    _resetStepStats();
    gamestate.held_direction = UNKNOWN;
    gamestate.held_frames = 0;
    gamestate.run_game = true;
}

// ************* INPUT HISTORY ******************//
// Update() runs once per frame, so counting calls here counts frames.

void _resetInputHistory()
{
    InputHistory empty = {0};
    gamestate.input_history = empty;
}

void _recordInput(GameDirection direction)
{
    InputHistory *h = &gamestate.input_history;
    InputHistoryEntry *newest = &h->entries[h->head];

    // Same direction as last frame: it's still being held
    if (h->count > 0 && newest->direction == direction)
    {
        if (newest->frames < INPUT_HISTORY_MAX_FRAMES)
            newest->frames += 1;
        return;
    }

    // New direction: push a new entry, overwriting the oldest when full
    if (h->count > 0)
        h->head = (h->head + 1) % INPUT_HISTORY_SIZE;
    if (h->count < INPUT_HISTORY_SIZE)
        h->count += 1;

    h->entries[h->head].direction = direction;
    h->entries[h->head].frames = 1;
    h->entries[h->head].missed = false;
}

void _markLastInputMissed()
{
    InputHistory *h = &gamestate.input_history;
    if (h->count > 0)
        h->entries[h->head].missed = true;
}

const InputHistoryEntry *GetInputHistoryEntry(int age)
{
    const InputHistory *h = &gamestate.input_history;
    if (age < 0 || age >= h->count)
        return NULL;

    return &h->entries[(h->head - age + INPUT_HISTORY_SIZE) % INPUT_HISTORY_SIZE];
}

// ************* STEP TIMING STATS ******************//

void _resetStepStats()
{
    for (int i = 0; i < MAX_PATTERN_SIZE; i++)
    {
        StepStats empty = {0};
        gamestate.step_stats[i] = empty;
    }
}

void _recordStepFrames(int step, int frames)
{
    if (step < 0 || step >= MAX_PATTERN_SIZE || frames <= 0)
        return;

    StepStats *s = &gamestate.step_stats[step];
    if (s->count > 0)
        s->head = (s->head + 1) % STEP_STATS_WINDOW;
    if (s->count < STEP_STATS_WINDOW)
        s->count += 1;
    s->samples[s->head] = frames;
}

StepSummary GetStepSummary(int step)
{
    StepSummary summary = {0};
    if (step < 0 || step >= MAX_PATTERN_SIZE)
        return summary;

    const StepStats *s = &gamestate.step_stats[step];
    if (s->count == 0)
        return summary;

    int total = 0;
    summary.count = s->count;
    summary.last = s->samples[s->head];
    summary.min = summary.last;
    summary.max = summary.last;
    for (int i = 0; i < s->count; i++)
    {
        int frames = s->samples[(s->head - i + STEP_STATS_WINDOW) % STEP_STATS_WINDOW];
        total += frames;
        if (frames < summary.min) summary.min = frames;
        if (frames > summary.max) summary.max = frames;
    }
    summary.avg = (float)total / s->count;

    return summary;
}

void DestroyGame()
{
    for(int i = 0; i < GAME_MODE_COUNT; i++)
    {
        free(gamemodes[i].pattern);
    }
}

// KBD steps with timing stats: neutral, back (before the downback cancel), downback
static const int _kbdStatSteps[] = {1, 2, 3};

void _initGameModes()
{
    GameDirection *pattern;

    // P1 KBD
    pattern = malloc(4 * sizeof(GameMode));
    pattern[0] = BACK;
    pattern[1] = NEUTRAL;
    pattern[2] = BACK;
    pattern[3] = DOWN_BACK;

    gamemodes[0].mode_name = "P1 KBD";
    gamemodes[0].pattern_size = 4;
    gamemodes[0].pattern = pattern;
    gamemodes[0].stat_steps = _kbdStatSteps;
    gamemodes[0].stat_step_count = 3;
    
    // P2 KBD
    pattern = malloc(4 * sizeof(GameMode));
    pattern[0] = FORWARD;
    pattern[1] = NEUTRAL;
    pattern[2] = FORWARD;
    pattern[3] = DOWN_FORWARD;

    gamemodes[1].mode_name = "P2 KBD";
    gamemodes[1].pattern_size = 4;
    gamemodes[1].pattern = pattern;
    gamemodes[1].stat_steps = _kbdStatSteps;
    gamemodes[1].stat_step_count = 3;

    // P1 WD
    pattern = malloc(6 * sizeof(GameMode));
    pattern[0] = FORWARD;
    pattern[1] = NEUTRAL;
    pattern[2] = DOWN;
    pattern[3] = DOWN_FORWARD;
    pattern[4] = FORWARD;
    pattern[5] = NEUTRAL;

    gamemodes[2].mode_name = "P1 WD";
    gamemodes[2].pattern_size = 6;
    gamemodes[2].pattern = pattern;


    // P2 WD
    pattern = malloc(6 * sizeof(GameMode));
    pattern[0] = BACK;
    pattern[1] = NEUTRAL;
    pattern[2] = DOWN;
    pattern[3] = DOWN_BACK;
    pattern[4] = BACK;
    pattern[5] = NEUTRAL;

    gamemodes[3].mode_name = "P2 WD";
    gamemodes[3].pattern_size = 6;
    gamemodes[3].pattern = pattern;

    printf("Gamemodes Initialized.\n");
}