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

static const char * _directionAssets[9] = {
    [0] = "assets/n.bmp",
    [1] = "assets/u.bmp",
    [2] = "assets/uf.bmp",
    [3] = "assets/f.bmp",
    [4] = "assets/df.bmp",
    [5] = "assets/d.bmp",
    [6] = "assets/db.bmp",
    [7] = "assets/b.bmp",
    [8] = "assets/ub.bmp"
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
