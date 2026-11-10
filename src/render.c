#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_image/SDL_image.h>

#include "render.h"
#include "game.h"
#include "input.h"


int ViewWidth = INITIAL_VIEW_WIDTH; 
int ViewHeight = WINDOW_HEIGHT; 

SDL_Texture *direction_textures[9];


// Score stuff
uint64_t curr_score = -1;
uint64_t curr_highscore = -2;

TTF_Font *score_font;
SDL_Texture *score_texture;
SDL_Texture *highscore_texture;

// Input accuracy stuff
SDL_Texture *acc_textures[3];

// Menu stuff
SDL_Texture *menu_textures[GAME_MODE_COUNT];

// Input history and stats stuff: one texture per glyph so numbers can be
// drawn every frame without making new textures
static const char _numberGlyphs[] = "0123456789.-x()";
#define NUMBER_GLYPH_COUNT (sizeof(_numberGlyphs) - 1)
SDL_Texture *glyph_textures[NUMBER_GLYPH_COUNT];

// Column headers for the timing stats panel
static const char *_statsLabels[STATS_COLUMN_COUNT] = {"LAST", "AVG", "MIN", "MAX"};
SDL_Texture *stats_label_textures[STATS_COLUMN_COUNT];

// Names for the random movement string list
SDL_Texture *move_name_textures[MOVE_TYPE_COUNT];



#ifdef KBD_EMBED_ASSETS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Assets are compiled into the exe as resources (see cmake/assets.rc.in).
// The resource name is the asset path in upper case with every other
// character replaced by '_', e.g. "assets/n.png" -> "ASSETS_N_PNG".
static SDL_IOStream *_openAsset(const char *relative)
{
    char name[256];
    size_t i = 0;
    for (; relative[i] && i < sizeof(name) - 1; i++)
    {
        char c = relative[i];
        if (c >= 'a' && c <= 'z')
            c = (char)(c - 'a' + 'A');
        else if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')))
            c = '_';
        name[i] = c;
    }
    name[i] = '\0';

    HRSRC res = FindResourceA(NULL, name, MAKEINTRESOURCEA(10) /* RT_RCDATA */);
    HGLOBAL data = res ? LoadResource(NULL, res) : NULL;
    const void *bytes = data ? LockResource(data) : NULL;
    if (bytes == NULL)
    {
        SDL_SetError("Embedded asset %s not found", name);
        return NULL;
    }
    return SDL_IOFromConstMem(bytes, SizeofResource(NULL, res));
}
#else
// Assets live next to the executable, so the game runs from any working directory
static SDL_IOStream *_openAsset(const char *relative)
{
    char path[1024];
    const char *base = SDL_GetBasePath();
    SDL_snprintf(path, sizeof(path), "%s%s", base ? base : "", relative);
    return SDL_IOFromFile(path, "rb");
}
#endif

// Initialize Textures
bool InitTextures(SDL_Renderer *renderer)
{
    SDL_Texture *curr = NULL;

    for (int i = 0; i < sizeof(_directionAssets) / sizeof(const char*); i++)
    {
        curr = IMG_LoadTexture_IO(renderer, _openAsset(_directionAssets[i]), true);    
        if (curr == NULL)
        {
            printf("Error loading asset(%s): %s\n", _directionAssets[i], SDL_GetError());
            return false;
        }

        direction_textures[i] = curr;
    }

    // Load font
    highscore_texture = NULL;
    score_texture = NULL;

    score_font = TTF_OpenFontIO(_openAsset(_fontAsset), true, 24);
    if (score_font == NULL)
    {
        printf("Error loading font asset(%s): %s\n", _fontAsset, SDL_GetError());
        return false;
    }

    // Build input acc result textures
    SDL_Surface *surface;
    acc_textures[0] = NULL;
    
    SDL_Color successColor =  {51, 255, 51};
    const char *successText = "GREAT";
    surface = TTF_RenderText_Solid(score_font, successText, strlen(successText), successColor);
    acc_textures[SUCCESS] = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    
    SDL_Color failColor = {255, 51, 51};
    const char *failText = "MISS";
    surface = TTF_RenderText_Solid(score_font, failText, strlen(failText), failColor);
    acc_textures[FAIL] = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);

    // Build glyph textures for the input history and stats numbers
    SDL_Color textColor = {255, 255, 255};
    for (int i = 0; i < NUMBER_GLYPH_COUNT; i++)
    {
        surface = TTF_RenderText_Solid(score_font, &_numberGlyphs[i], 1, textColor);
        glyph_textures[i] = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_DestroySurface(surface);

        if (glyph_textures[i] == NULL)
        {
            printf("Error building glyph texture: %s\n", SDL_GetError());
            return false;
        }
    }

    // Build the stats column headers
    for (int i = 0; i < STATS_COLUMN_COUNT; i++)
    {
        surface = TTF_RenderText_Solid(score_font, _statsLabels[i], strlen(_statsLabels[i]), textColor);
        stats_label_textures[i] = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_DestroySurface(surface);

        if (stats_label_textures[i] == NULL)
        {
            printf("Error building stats label texture: %s\n", SDL_GetError());
            return false;
        }
    }

    // Build the move names for the random movement string mode
    for (int i = 0; i < MOVE_TYPE_COUNT; i++)
    {
        surface = TTF_RenderText_Solid(score_font, move_info[i].name, strlen(move_info[i].name), textColor);
        move_name_textures[i] = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_DestroySurface(surface);

        if (move_name_textures[i] == NULL)
        {
            printf("Error building move name texture: %s\n", SDL_GetError());
            return false;
        }
    }
 
    return true;
}

void _updateScore(SDL_Renderer *renderer)
{
    char scoreText[32];
    SDL_Color scoreColor = {255, 255, 255};
    if (gamestate.score == curr_score && gamestate.highscore == curr_highscore)
        return;
    
    // Update score and destroy old texture
    curr_score = gamestate.score;

    if (score_texture != NULL)
        SDL_DestroyTexture(score_texture);
    
    
    snprintf(scoreText, 32, "%06" PRIu64, gamestate.score);
    SDL_Surface *scoreSurface = TTF_RenderText_Solid(score_font, scoreText, strlen(scoreText), scoreColor);
    
    score_texture = SDL_CreateTextureFromSurface(renderer, scoreSurface);
    SDL_DestroySurface(scoreSurface);
    
    if (gamestate.highscore == curr_highscore)
        return;
    
    // Update high score and destroy old texture
    curr_highscore = gamestate.highscore;
    if (highscore_texture != NULL)
    SDL_DestroyTexture(highscore_texture);
    
    snprintf(scoreText, 32, "%06" PRIu64, gamestate.highscore);
    scoreSurface = TTF_RenderText_Solid(score_font, scoreText, strlen(scoreText), scoreColor);
    
    highscore_texture = SDL_CreateTextureFromSurface(renderer, scoreSurface);
    SDL_DestroySurface(scoreSurface);
}



// Initialize the view for mode select
bool InitMenuTextures(SDL_Renderer *renderer)
{
    char text[32];
    
    SDL_Color textColor = {255, 255, 255};
    
    for (int i = 0; i < GAME_MODE_COUNT; i++)
    {
        snprintf(text, 32, "%s", gamemodes[i].mode_name);
        SDL_Surface *surface = TTF_RenderText_Solid(score_font, text, strlen(text), textColor);
        
        menu_textures[i] = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_DestroySurface(surface);
        
        if (menu_textures[i] == NULL )
        {
            printf("Error initializing mode select view: %s", SDL_GetError());
            return false;
        }
    }
    
    return true;
}

// Destroy mode select textures from memory after game starts
void DestroyMenuTextures()
{
    for (int i = 0; i < GAME_MODE_COUNT; i++)
    SDL_DestroyTexture(menu_textures[i]);
}

void Render(SDL_Renderer *renderer)
{
    if (gamestate.run_game) 
        _renderGame(renderer);
    else
        _renderMenu(renderer);
}

void _renderMenu(SDL_Renderer *renderer)
{
    SDL_RenderClear(renderer);
    
    // Keep the mode name the same size as before, centered in the wider window
    SDL_FRect destRect = {(INITIAL_VIEW_WIDTH - GAME_AREA_WIDTH) / 2 + SIDE_PADDING, WINDOW_HEIGHT / 2 - ICON_HEIGHT / 2, GAME_AREA_WIDTH - (SIDE_PADDING * 2), ICON_HEIGHT};
    SDL_RenderTexture(renderer, menu_textures[selected_mode], NULL, &destRect);
    
    SDL_RenderPresent(renderer);
}

// ************* GAME RENDER ******************//
SDL_FRect next_input_rect = {
    SIDE_PADDING,
    (INITIAL_VIEW_HEIGHT / 2) - (ICON_HEIGHT / 2),
    ICON_WIDTH,
    ICON_HEIGHT
};

SDL_FRect score_rect = {
    ICON_WIDTH + SIDE_PADDING * 2,
    VERT_PADDING,
    SCORE_COUNTER_WIDTH,
    SCORE_COUNTER_HEIGHT
};

SDL_FRect highscore_rect = {
    ICON_WIDTH + SIDE_PADDING * 2,
    SCORE_COUNTER_HEIGHT + VERT_PADDING,
    SCORE_COUNTER_WIDTH,
    SCORE_COUNTER_HEIGHT
};

SDL_FRect last_input_acc_rect = {
    (ICON_WIDTH + SIDE_PADDING * 2) / 2 - ACC_DISPLAY_WIDTH / 2,
    INITIAL_VIEW_HEIGHT - ACC_DISPLAY_HEIGHT - VERT_PADDING,
    ACC_DISPLAY_WIDTH,
    ACC_DISPLAY_HEIGHT
};

SDL_FRect failed_input_rect = {
    (ICON_WIDTH + SIDE_PADDING * 2) / 2 - ACC_DISPLAY_WIDTH / 2,
    INITIAL_VIEW_HEIGHT - VERT_PADDING - 27,
    24,
    24
};

SDL_FRect failed_acc_rect = {
    (ICON_WIDTH + SIDE_PADDING * 2) / 2 - ACC_DISPLAY_WIDTH / 2 + 30,
    INITIAL_VIEW_HEIGHT - ACC_DISPLAY_HEIGHT - VERT_PADDING,
    ACC_DISPLAY_WIDTH - 30,
    ACC_DISPLAY_HEIGHT
};


// Draws a number (digits, '.' and '-') with its right edge at rightX
static void _drawNumber(SDL_Renderer *renderer, const char *text, float rightX, float y, float height, SDL_Color color)
{
    float x = rightX;
    for (int i = (int)strlen(text) - 1; i >= 0; i--)
    {
        const char *glyph = strchr(_numberGlyphs, text[i]);
        if (glyph == NULL || *glyph == '\0')
            continue;

        SDL_Texture *texture = glyph_textures[glyph - _numberGlyphs];
        float w, h;
        SDL_GetTextureSize(texture, &w, &h);

        SDL_FRect rect = {0, y, w * height / h, height};
        x -= rect.w;
        rect.x = x;

        SDL_SetTextureColorMod(texture, color.r, color.g, color.b);
        SDL_RenderTexture(renderer, texture, NULL, &rect);
    }
}

// Draws a texture scaled to `height` with its right edge at rightX
static void _drawRightAligned(SDL_Renderer *renderer, SDL_Texture *texture, float rightX, float y, float height, SDL_Color color)
{
    float w, h;
    SDL_GetTextureSize(texture, &w, &h);
    SDL_FRect rect = {rightX - w * height / h, y, w * height / h, height};
    SDL_SetTextureColorMod(texture, color.r, color.g, color.b);
    SDL_RenderTexture(renderer, texture, NULL, &rect);
}

// Draws the most recent inputs, newest on top, each with how many frames it was held
void _renderInputHistory(SDL_Renderer *renderer)
{
    const float panelX = GAME_AREA_WIDTH;

    // Divider between the play area and the history
    SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
    SDL_RenderLine(renderer, panelX - SIDE_PADDING / 2, VERT_PADDING, panelX - SIDE_PADDING / 2, INITIAL_VIEW_HEIGHT - VERT_PADDING);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    for (int row = 0; row < HISTORY_ROWS; row++)
    {
        const InputHistoryEntry *entry = GetInputHistoryEntry(row);
        if (entry == NULL)
            break;

        float rowY = VERT_PADDING + row * HISTORY_ROW_HEIGHT;

        // Direction icon (UNKNOWN / DISCONNECTED have no icon)
        if (entry->direction >= NEUTRAL && entry->direction <= UP_BACK)
        {
            SDL_FRect iconRect = {
                panelX,
                rowY + (HISTORY_ROW_HEIGHT - HISTORY_ICON_SIZE) / 2,
                HISTORY_ICON_SIZE,
                HISTORY_ICON_SIZE
            };
            // Tint the input that caused a MISS red. The icon textures are
            // shared with the next-input display, so reset the tint after.
            SDL_Texture *icon = direction_textures[entry->direction];
            if (entry->missed)
                SDL_SetTextureColorMod(icon, 255, 51, 51);
            SDL_RenderTexture(renderer, icon, NULL, &iconRect);
            SDL_SetTextureColorMod(icon, 255, 255, 255);
        }

        // Frame count, right aligned. Newest input is white, older ones grey,
        // and the input that caused a MISS is red.
        char frames[8];
        snprintf(frames, sizeof(frames), "%d", entry->frames);
        Uint8 shade = row == 0 ? 255 : 160;
        SDL_Color digitColor = entry->missed ? (SDL_Color){255, 51, 51, 255} : (SDL_Color){shade, shade, shade, 255};

        _drawNumber(renderer, frames, panelX + HISTORY_PANEL_WIDTH, rowY + (HISTORY_ROW_HEIGHT - HISTORY_DIGIT_HEIGHT) / 2, HISTORY_DIGIT_HEIGHT, digitColor);
    }
}

// Draws how many frames each tracked pattern step was held over the recent reps:
// the last rep, and the average, min and max over the last STEP_STATS_WINDOW reps
void _renderStepStats(SDL_Renderer *renderer)
{
    const GameMode *mode = gamestate.current_mode;
    if (mode->stat_step_count == 0)
        return;

    const float panelY = INITIAL_VIEW_HEIGHT;
    const float textOffset = (STATS_ROW_HEIGHT - STATS_TEXT_HEIGHT) / 2;
    const SDL_Color labelColor = {160, 160, 160, 255};
    const SDL_Color valueColor = {255, 255, 255, 255};

    // Divider between the play area and the stats
    SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
    SDL_RenderLine(renderer, SIDE_PADDING, panelY, INITIAL_VIEW_WIDTH - SIDE_PADDING, panelY);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    // Column headers
    for (int col = 0; col < STATS_COLUMN_COUNT; col++)
    {
        float right = STATS_FIRST_COLUMN_RIGHT + col * STATS_COLUMN_WIDTH;
        _drawRightAligned(renderer, stats_label_textures[col], right, panelY + textOffset, STATS_TEXT_HEIGHT, labelColor);
    }

    for (int row = 0; row < mode->stat_step_count && row < STATS_ROWS; row++)
    {
        int step = mode->stat_steps[row];
        float rowY = panelY + (row + 1) * STATS_ROW_HEIGHT;

        // Icon for the step's direction
        SDL_FRect iconRect = {
            SIDE_PADDING,
            rowY + (STATS_ROW_HEIGHT - STATS_ICON_SIZE) / 2,
            STATS_ICON_SIZE,
            STATS_ICON_SIZE
        };
        SDL_RenderTexture(renderer, direction_textures[mode->pattern[step]], NULL, &iconRect);

        StepSummary summary = GetStepSummary(step);
        char values[STATS_COLUMN_COUNT][16];
        if (summary.count == 0)
        {
            for (int col = 0; col < STATS_COLUMN_COUNT; col++)
                snprintf(values[col], sizeof(values[col]), "-");
        }
        else
        {
            snprintf(values[0], sizeof(values[0]), "%d", summary.last);
            snprintf(values[1], sizeof(values[1]), "%.1f", summary.avg);
            snprintf(values[2], sizeof(values[2]), "%d", summary.min);
            snprintf(values[3], sizeof(values[3]), "%d", summary.max);
        }

        for (int col = 0; col < STATS_COLUMN_COUNT; col++)
        {
            float right = STATS_FIRST_COLUMN_RIGHT + col * STATS_COLUMN_WIDTH;
            _drawNumber(renderer, values[col], right, rowY + textOffset, STATS_TEXT_HEIGHT, valueColor);
        }
    }
}

// Width of a number drawn with _drawNumber at `height`
static float _numberWidth(const char *text, float height)
{
    float total = 0;
    for (int i = 0; text[i]; i++)
    {
        const char *glyph = strchr(_numberGlyphs, text[i]);
        if (glyph == NULL || *glyph == '\0')
            continue;
        float w, h;
        SDL_GetTextureSize(glyph_textures[glyph - _numberGlyphs], &w, &h);
        total += w * height / h;
    }
    return total;
}

// Draws one row of the movement string: its name (with "(xN)" when it
// repeats), and the inputs of one rep under it. For the current row, N
// counts the reps left, the inputs are for the rep being done (a KBD after
// the first one has no back dash) and inputs already done are dimmed.
static void _renderMoveRow(SDL_Renderer *renderer, const MoveRow *row, int rep, float y, bool current, int stepsDone, bool missed)
{
    MoveType move = GetRowMove(row, rep);
    const MoveInfo *info = &move_info[move];
    MoveType nameMove = row->type;

    SDL_Color nameColor = current ? (SDL_Color){255, 255, 255, 255} : (SDL_Color){150, 150, 150, 255};
    if (missed)
        nameColor = (SDL_Color){255, 51, 51, 255};

    SDL_Texture *name = move_name_textures[nameMove];
    float w, h;
    SDL_GetTextureSize(name, &w, &h);
    char count[16] = "";
    if (row->reps > 1)
        snprintf(count, sizeof(count), "(x%d)", row->reps - rep);
    const float countGap = count[0] ? 4 : 0;

    // Shrink names that don't fit the row, keeping them on the same baseline
    float height = MOVES_NAME_HEIGHT;
    float totalW = w * height / h + countGap + _numberWidth(count, height);
    const float maxW = MOVES_LIST_WIDTH - 12;
    if (totalW > maxW)
        height = height * maxW / totalW;

    SDL_FRect nameRect = {MOVES_LIST_X + 8, y + 3 + MOVES_NAME_HEIGHT - height, w * height / h, height};
    SDL_SetTextureColorMod(name, nameColor.r, nameColor.g, nameColor.b);
    SDL_RenderTexture(renderer, name, NULL, &nameRect);

    if (count[0])
    {
        float right = nameRect.x + nameRect.w + countGap + _numberWidth(count, height);
        _drawNumber(renderer, count, right, nameRect.y, height, nameColor);
    }

    float iconX = MOVES_LIST_X + 8;
    float iconY = y + MOVES_NAME_HEIGHT + 6;
    for (int i = 0; i < info->step_count; i++)
    {
        SDL_Texture *icon = direction_textures[GetMoveStep(move, i)];
        SDL_FRect iconRect = {iconX, iconY, MOVES_ICON_SIZE, MOVES_ICON_SIZE};

        Uint8 shade = 255;
        if (!current)
            shade = 150;
        else if (i < stepsDone)
            shade = 90;
        SDL_SetTextureColorMod(icon, shade, shade, shade);
        SDL_RenderTexture(renderer, icon, NULL, &iconRect);
        SDL_SetTextureColorMod(icon, 255, 255, 255);

        iconX += MOVES_ICON_SIZE + 3;
    }

    // Side step: a bar showing how long the input is held. It has to be let
    // go before the bar fills.
    if (info->hold != HOLD_TAP)
        return;

    const float barX = iconX + 4;
    const float barW = MOVES_LIST_WIDTH - (barX - MOVES_LIST_X) - 8;
    const float barH = 5;
    const float barY = iconY + (MOVES_ICON_SIZE - barH) / 2;

    SDL_SetRenderDrawColor(renderer, 70, 70, 70, 255);
    SDL_FRect track = {barX, barY, barW, barH};
    SDL_RenderRect(renderer, &track);

    if (current && stepsDone == info->step_count)
    {
        float fill = (float)gamestate.held_frames / SIDE_STEP_FRAMES;
        if (fill > 1)
            fill = 1;
        SDL_SetRenderDrawColor(renderer, 230, 200, 51, 255);
        SDL_FRect bar = {barX, barY, barW * fill, barH};
        SDL_RenderFillRect(renderer, &bar);
    }
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
}

// Draws the next MOVES_SHOWN rows, current one on top. When a row is
// finished the list scrolls up one row.
void _renderMoves(SDL_Renderer *renderer)
{
    static int seenRowsDone = 0;
    static float scroll = 0;

    const MoveString *m = &gamestate.moves;
    if (m->rows_done != seenRowsDone)
    {
        scroll = m->rows_done > seenRowsDone ? MOVES_ROW_HEIGHT : 0;
        seenRowsDone = m->rows_done;
    }

    // Box around the current move
    SDL_FRect currentRect = {MOVES_LIST_X, MOVES_LIST_Y, MOVES_LIST_WIDTH, MOVES_ROW_HEIGHT - 2};
    SDL_SetRenderDrawColor(renderer, 35, 35, 35, 255);
    SDL_RenderFillRect(renderer, &currentRect);
    if (gamestate.last_input_acc == FAIL)
        SDL_SetRenderDrawColor(renderer, 255, 51, 51, 255);
    else
        SDL_SetRenderDrawColor(renderer, 120, 120, 120, 255);
    SDL_RenderRect(renderer, &currentRect);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    SDL_Rect clip = {MOVES_LIST_X, MOVES_LIST_Y, MOVES_LIST_WIDTH, MOVES_ROW_HEIGHT * MOVES_SHOWN};
    SDL_SetRenderClipRect(renderer, &clip);

    // The row just finished scrolls out the top
    if (scroll > 0)
        _renderMoveRow(renderer, &m->last_row, 0, MOVES_LIST_Y + scroll - MOVES_ROW_HEIGHT, false, 0, false);

    for (int i = 0; i < MOVES_SHOWN; i++)
    {
        bool current = i == 0;
        _renderMoveRow(renderer, &m->queue[i], current ? m->rep : 0, MOVES_LIST_Y + i * MOVES_ROW_HEIGHT + scroll,
            current, current ? m->step : 0, current && gamestate.last_input_acc == FAIL);
    }

    SDL_SetRenderClipRect(renderer, NULL);

    scroll -= (float)MOVES_ROW_HEIGHT / MOVES_SCROLL_FRAMES;
    if (scroll < 0)
        scroll = 0;
}

static void _renderMovesGame(SDL_Renderer *renderer)
{
    _renderMoves(renderer);

    // Score and high score, to the right of the list
    _updateScore(renderer);
    SDL_FRect scoreRect = {MOVES_SCORE_X, VERT_PADDING, MOVES_SCORE_WIDTH, MOVES_SCORE_HEIGHT};
    SDL_RenderTexture(renderer, score_texture, NULL, &scoreRect);
    SDL_FRect highRect = {MOVES_SCORE_X, VERT_PADDING + MOVES_SCORE_HEIGHT + 4, MOVES_SCORE_WIDTH, MOVES_SCORE_HEIGHT};
    SDL_SetTextureColorMod(highscore_texture, 150, 150, 150);
    SDL_RenderTexture(renderer, highscore_texture, NULL, &highRect);
    SDL_SetTextureColorMod(highscore_texture, 255, 255, 255);

    _renderInputHistory(renderer);

    if (gamestate.last_input_acc == FAIL)
    {
        float y = VERT_PADDING + MOVES_SCORE_HEIGHT * 2 + 20;
        SDL_FRect iconRect = {MOVES_SCORE_X, y + 3, 24, 24};
        if (gamestate.last_input >= NEUTRAL && gamestate.last_input <= UP_BACK)
            SDL_RenderTexture(renderer, direction_textures[gamestate.last_input], NULL, &iconRect);
        SDL_FRect accRect = {MOVES_SCORE_X + 28, y, MOVES_SCORE_WIDTH - 28, ACC_DISPLAY_HEIGHT};
        SDL_RenderTexture(renderer, acc_textures[FAIL], NULL, &accRect);

        SDL_RenderPresent(renderer);

        // Same pause as the other modes. Multiple delay calls to prevent
        // Windows from "not responding"
        SDL_DelayPrecise(500000000);
        SDL_DelayPrecise(500000000);
        SDL_DelayPrecise(200000000);
        return;
    }

    SDL_RenderPresent(renderer);
}

void _renderGame(SDL_Renderer *renderer) 
{
    if (gamestate.current_mode->random_moves)
    {
        SDL_RenderClear(renderer);
        _renderMovesGame(renderer);
        return;
    }

    SDL_RenderClear(renderer);
    
    // Render inputs
    SDL_Texture *nextInputTexture = direction_textures[ gamestate.current_mode->pattern[ gamestate.player_pos % gamestate.current_mode->pattern_size ] ];
    SDL_RenderTexture(renderer, nextInputTexture, NULL, &next_input_rect);
    
    // Render score and high score panel
    _updateScore(renderer);
    SDL_RenderTexture(renderer, score_texture, NULL, &score_rect);
    SDL_RenderTexture(renderer, highscore_texture, NULL, &highscore_rect);

    // Input history panel
    _renderInputHistory(renderer);

    // Timing stats panel
    _renderStepStats(renderer);
    
    // Success fail panel
    if (gamestate.last_input_acc == FAIL)
    {
        // First the arrow on the left
        SDL_RenderTexture(renderer, direction_textures[ gamestate.last_input ], NULL, &failed_input_rect);
        SDL_RenderTexture(renderer, acc_textures[ gamestate.last_input_acc ], NULL, &failed_acc_rect);
        
        SDL_RenderPresent(renderer);

        // Multiple delay calls to prevent Windows from "not responding"
        SDL_DelayPrecise(500000000);
        SDL_DelayPrecise(500000000);
        SDL_DelayPrecise(200000000);

        return;
    }
    else if (gamestate.last_input_acc != NONE)
    {
        /* TODO: Figure out how to display success in a way that doesn't look fucking stupid */
        //SDL_RenderTexture(renderer, acc_textures[ gamestate.last_input_acc ], NULL, &last_input_acc_rect);
    }

    SDL_RenderPresent(renderer);
}
