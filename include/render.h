#pragma once

#include <SDL3/SDL.h>

#include "input.h"
#include "game.h"

#define ICON_WIDTH 70
#define ICON_HEIGHT 70

#define SIDE_PADDING 30
#define VERT_PADDING 15

#define SCORE_COUNTER_WIDTH 200
#define SCORE_COUNTER_HEIGHT 70

#define ACC_DISPLAY_HEIGHT 30
#define ACC_DISPLAY_WIDTH 100

// Width of the original play area (next input + score), without the history panel
#define GAME_AREA_WIDTH (ICON_WIDTH + SCORE_COUNTER_WIDTH + (SIDE_PADDING * 3))
#define INITIAL_VIEW_HEIGHT (ICON_HEIGHT * 2 + VERT_PADDING * 2)

// Input history panel, on the right side of the game view
#define HISTORY_ROW_HEIGHT 20
#define HISTORY_ICON_SIZE 18
#define HISTORY_DIGIT_HEIGHT 16
#define HISTORY_PANEL_WIDTH 70
#define HISTORY_ROWS ((INITIAL_VIEW_HEIGHT - VERT_PADDING * 2) / HISTORY_ROW_HEIGHT)

#define INITIAL_VIEW_WIDTH (GAME_AREA_WIDTH + HISTORY_PANEL_WIDTH + SIDE_PADDING)

// Timing stats panel, below the play area and history
#define STATS_ROW_HEIGHT 20
#define STATS_ICON_SIZE 18
#define STATS_TEXT_HEIGHT 16
// Most stat rows a mode shows
#define STATS_ROWS 3
#define STATS_PANEL_HEIGHT (STATS_ROW_HEIGHT * (STATS_ROWS + 1) + VERT_PADDING)
// Right edge of the first value column and the width of each column
#define STATS_FIRST_COLUMN_RIGHT (SIDE_PADDING + 115)
#define STATS_COLUMN_WIDTH 90
#define STATS_COLUMN_COUNT 4

#define WINDOW_HEIGHT (INITIAL_VIEW_HEIGHT + STATS_PANEL_HEIGHT)

// Random movement string list, on the left of the game view
#define MOVES_ROW_HEIGHT 44
#define MOVES_LIST_X SIDE_PADDING
#define MOVES_LIST_Y VERT_PADDING
#define MOVES_LIST_WIDTH 200
#define MOVES_NAME_HEIGHT 16
#define MOVES_ICON_SIZE 22
// Frames the list takes to scroll up one row
#define MOVES_SCROLL_FRAMES 6
// Score panel to the right of the list
#define MOVES_SCORE_X (MOVES_LIST_X + MOVES_LIST_WIDTH + 15)
#define MOVES_SCORE_WIDTH (GAME_AREA_WIDTH - SIDE_PADDING / 2 - 15 - MOVES_SCORE_X)
#define MOVES_SCORE_HEIGHT 31

static const char * _directionAssets[9] = {
    [0] = "assets/n.png",
    [1] = "assets/u.png",
    [2] = "assets/uf.png",
    [3] = "assets/f.png",
    [4] = "assets/df.png",
    [5] = "assets/d.png",
    [6] = "assets/db.png",
    [7] = "assets/b.png",
    [8] = "assets/ub.png"
};

static const char * _playerAsset = "assets/ez1.png";
static const char * _fontAsset = "assets/scorefont.ttf";

bool InitTextures(SDL_Renderer *);

bool InitMenuTextures(SDL_Renderer *);
void DestroyMenuTextures();

void Render(SDL_Renderer *);
void _renderMenu(SDL_Renderer *);
void _renderGame(SDL_Renderer *);
void _playFailAnimation(SDL_Renderer *);

void _updateScore(SDL_Renderer *rendrerer);
void _renderInputHistory(SDL_Renderer *);
void _renderStepStats(SDL_Renderer *);
void _renderMoves(SDL_Renderer *);
