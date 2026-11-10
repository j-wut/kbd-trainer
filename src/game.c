#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>

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
        if (gamestate.current_mode->random_moves)
            _resetMoves();
        prev_input = *cs;
        return;
    }

    if (cs->back_pressed)
    {
        highscores[selected_mode] = gamestate.highscore;
        gamestate.run_game = false;
        return;
    }

    if (gamestate.current_mode->random_moves)
    {
        _updateMoves(cs);
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
    if (gamestate.current_mode->random_moves)
        _resetMoves();
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

// ************* RANDOM MOVEMENT STRING ******************//

const MoveInfo move_info[MOVE_TYPE_COUNT] = {
    [MOVE_BACK_DASH]      = {"BACKDASH",      {BACK, NEUTRAL, BACK}, 3, HOLD_ANY},
    [MOVE_KBD]            = {"KBD",           {BACK, DOWN_BACK, BACK, NEUTRAL, BACK}, 5, HOLD_ANY},
    [MOVE_SIDE_STEP_UP]   = {"SSL",           {UP}, 1, HOLD_TAP},
    [MOVE_SIDE_WALK_UP]   = {"SWL",           {UP, NEUTRAL, UP}, 3, HOLD_ANY},
    [MOVE_SIDE_STEP_DOWN] = {"SSR",           {DOWN}, 1, HOLD_TAP},
    [MOVE_SIDE_WALK_DOWN] = {"SWR",           {DOWN, NEUTRAL, DOWN}, 3, HOLD_ANY},
    [MOVE_FORWARD_DASH]   = {"DASH",          {FORWARD, NEUTRAL, FORWARD}, 3, HOLD_ANY},
    [MOVE_WAVE_DASH]      = {"WVD",           {FORWARD, NEUTRAL, DOWN, DOWN_FORWARD, FORWARD}, 5, HOLD_ANY},
    [MOVE_BLOCK]          = {"BLOCK",         {BACK}, 1, HOLD_ANY},
    [MOVE_BACK_DASH_KBD]  = {"BACKDASH KBD",  {BACK, NEUTRAL, BACK, DOWN_BACK, BACK, NEUTRAL, BACK}, 7, HOLD_ANY},
    [MOVE_DASH_WAVE_DASH] = {"DASH WVD",      {FORWARD, NEUTRAL, FORWARD, NEUTRAL, DOWN, DOWN_FORWARD, FORWARD}, 7, HOLD_ANY},
};

#define MOVE_SCORE 100

static GameDirection _mirror(GameDirection d)
{
    switch (d)
    {
        case FORWARD:      return BACK;
        case BACK:         return FORWARD;
        case UP_FORWARD:   return UP_BACK;
        case UP_BACK:      return UP_FORWARD;
        case DOWN_FORWARD: return DOWN_BACK;
        case DOWN_BACK:    return DOWN_FORWARD;
        default:           return d;
    }
}

GameDirection GetMoveStep(MoveType move, int step)
{
    GameDirection d = move_info[move].steps[step];
    return gamestate.current_mode->mirrored ? _mirror(d) : d;
}

// Bits for up / down / back / forward, to tell when one direction is
// part of another (releasing one key of a diagonal)
static int _directionBits(GameDirection d)
{
    switch (d)
    {
        case UP:           return 1;
        case DOWN:         return 2;
        case BACK:         return 4;
        case FORWARD:      return 8;
        case UP_BACK:      return 1 | 4;
        case UP_FORWARD:   return 1 | 8;
        case DOWN_BACK:    return 2 | 4;
        case DOWN_FORWARD: return 2 | 8;
        default:           return 0;
    }
}

MoveType GetNameMove(MoveType move)
{
    if (!gamestate.current_mode->mirrored)
        return move;
    switch (move)
    {
        case MOVE_SIDE_STEP_UP:   return MOVE_SIDE_STEP_DOWN;
        case MOVE_SIDE_STEP_DOWN: return MOVE_SIDE_STEP_UP;
        case MOVE_SIDE_WALK_UP:   return MOVE_SIDE_WALK_DOWN;
        case MOVE_SIDE_WALK_DOWN: return MOVE_SIDE_WALK_UP;
        default:                  return move;
    }
}

MoveType GetRowMove(const MoveRow *row, int rep)
{
    if (rep == 0)
        return row->type;
    if (row->type == MOVE_BACK_DASH_KBD)
        return MOVE_KBD;
    if (row->type == MOVE_DASH_WAVE_DASH)
        return MOVE_WAVE_DASH;
    return row->type;
}

// How many times a row repeats: 1, and then another rep a third of the time
static int _rollReps()
{
    int reps = 1;
    while (reps < MAX_ROW_REPS && rand() % 3 == 0)
        reps += 1;
    return reps;
}

// True for a side step and side walk in the same direction (either order)
static bool _sameSide(MoveType a, MoveType b)
{
    return (a == MOVE_SIDE_STEP_UP && b == MOVE_SIDE_WALK_UP)
        || (a == MOVE_SIDE_WALK_UP && b == MOVE_SIDE_STEP_UP)
        || (a == MOVE_SIDE_STEP_DOWN && b == MOVE_SIDE_WALK_DOWN)
        || (a == MOVE_SIDE_WALK_DOWN && b == MOVE_SIDE_STEP_DOWN);
}

// Picks the row to follow `prev` (MOVE_TYPE_COUNT for the first row):
// - back dash and the side steps / walks never repeat (two back dashes is a KBD),
//   and a side step and side walk in the same direction never follow each other
// - a KBD chain always starts with its back dash, as one BACKDASH KBD row
//   with the number of KBDs; a back dash never comes right after a KBD
// - a dash into a wave dash is always one DASH WVD row
// - wave dashes in a row are one row with the count, so a WVD row never
//   follows a WVD or DASH WVD row
// - two moves in a row that start with a dash need a block in between
static MoveRow _nextRow(MoveType prev)
{
    MoveString *m = &gamestate.moves;
    MoveRow row = {MOVE_TYPE_COUNT, 1};
    if (m->pending != MOVE_TYPE_COUNT)
    {
        row.type = m->pending;
        m->pending = MOVE_TYPE_COUNT;
        if (row.type == MOVE_DASH_WAVE_DASH)
            row.reps = _rollReps();
        return row;
    }

    for (;;)
    {
        MoveType next = (MoveType)(rand() % MOVE_BLOCK);

        if (next == MOVE_BACK_DASH && (prev == MOVE_BACK_DASH || prev == MOVE_BACK_DASH_KBD))
            continue;

        if (next == MOVE_KBD)
        {
            // A KBD right after a back dash or KBD would be part of that row
            if (prev == MOVE_BACK_DASH || prev == MOVE_BACK_DASH_KBD)
                continue;
            next = MOVE_BACK_DASH_KBD;
        }

        if (next == MOVE_WAVE_DASH)
        {
            // A dash followed by a wave dash is always the combined move
            if (prev == MOVE_FORWARD_DASH)
                continue;
            // Half of the wave dashes come with a dash in front
            if (rand() % 2)
                next = MOVE_DASH_WAVE_DASH;
            // More wave dashes go in the row before
            else if (prev == MOVE_WAVE_DASH || prev == MOVE_DASH_WAVE_DASH)
                continue;
        }

        if ((next == MOVE_FORWARD_DASH || next == MOVE_DASH_WAVE_DASH) && prev == MOVE_FORWARD_DASH)
        {
            m->pending = next;
            row.type = MOVE_BLOCK;
            return row;
        }

        // Side steps and walks don't repeat
        if ((next == prev && next != MOVE_DASH_WAVE_DASH) || _sameSide(prev, next))
            continue;

        row.type = next;
        if (next == MOVE_BACK_DASH_KBD || next == MOVE_WAVE_DASH || next == MOVE_DASH_WAVE_DASH)
            row.reps = _rollReps();
        return row;
    }
}

void _resetMoves()
{
    MoveString *m = &gamestate.moves;
    m->pending = MOVE_TYPE_COUNT;
    m->queue[0] = _nextRow(MOVE_TYPE_COUNT);
    for (int i = 1; i < MOVES_SHOWN; i++)
        m->queue[i] = _nextRow(m->queue[i - 1].type);
    m->last_row = m->queue[0];
    m->rep = 0;
    m->step = 0;
    m->moves_done = 0;
    m->rows_done = 0;
    m->release_from = NEUTRAL;
}

static MoveType _currentMove()
{
    const MoveString *m = &gamestate.moves;
    return GetRowMove(&m->queue[0], m->rep);
}

static void _completeMove()
{
    MoveString *m = &gamestate.moves;
    MoveType done = _currentMove();

    m->release_from = GetMoveStep(done, move_info[done].step_count - 1);
    m->step = 0;
    m->moves_done += 1;
    m->rep += 1;

    gamestate.score += MOVE_SCORE;
    if (gamestate.score > gamestate.highscore)
        gamestate.highscore = gamestate.score;
    gamestate.last_input_acc = SUCCESS;

    if (m->rep < m->queue[0].reps)
    {
        // The last back of a KBD counts as the first back of the next KBD:
        // KBD into KBD is b db b n b db b n b. Wave dashes don't chain, the
        // next one needs a neutral before its f.
        if (_currentMove() == MOVE_KBD)
            m->step = 1;
        return;
    }

    // Row finished: scroll to the next one
    m->last_row = m->queue[0];
    for (int i = 0; i < MOVES_SHOWN - 1; i++)
        m->queue[i] = m->queue[i + 1];
    m->queue[MOVES_SHOWN - 1] = _nextRow(m->queue[MOVES_SHOWN - 2].type);
    m->rep = 0;
    m->rows_done += 1;
}

static void _missMove(GameDirection direction)
{
    gamestate.last_input = direction;
    gamestate.last_input_acc = FAIL;
    _markLastInputMissed();
}

// A new direction was pressed (or released to)
static void _moveDirectionChanged(GameDirection direction)
{
    MoveString *m = &gamestate.moves;
    const MoveInfo *move = &move_info[_currentMove()];

    if (direction == GetMoveStep(_currentMove(), m->step))
    {
        m->step += 1;
        gamestate.last_input = direction;
        // Side steps and walks finish once the hold is known
        if (m->step == move->step_count && move->hold == HOLD_ANY)
            _completeMove();
        return;
    }

    if (m->step == 0)
    {
        // Between moves, going back to neutral or letting go of part of the
        // last input is fine
        if (direction == NEUTRAL || direction == UNKNOWN)
            return;
        int bits = _directionBits(direction);
        if ((bits & _directionBits(m->release_from)) == bits)
            return;

        // Wrong inputs before the first move of a run are ignored
        if (m->moves_done == 0)
            return;
    }

    _missMove(direction);
}

void _updateMoves(ControllerState *cs)
{
    MoveString *m = &gamestate.moves;
    const MoveInfo *move = &move_info[_currentMove()];
    bool changed = cs->direction != prev_input.direction;
    prev_input.direction = cs->direction;

    // A side step's input is down: it has to be let go in time
    if (m->step == move->step_count && move->hold == HOLD_TAP)
    {
        GameDirection held = GetMoveStep(_currentMove(), m->step - 1);
        bool long_enough = cs->direction == held && gamestate.held_frames >= SIDE_STEP_FRAMES;

        if (long_enough)
        {
            _missMove(cs->direction);
            return;
        }
        if (!changed)
            return;

        // Released in time. The new direction may start the next move.
        _completeMove();
    }

    if (changed)
        _moveDirectionChanged(cs->direction);
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

    // P1 / P2 random movement strings
    gamemodes[4].mode_name = "P1 MOVES";
    gamemodes[4].random_moves = true;

    gamemodes[5].mode_name = "P2 MOVES";
    gamemodes[5].random_moves = true;
    gamemodes[5].mirrored = true;

    srand((unsigned int)time(NULL));

    printf("Gamemodes Initialized.\n");
}