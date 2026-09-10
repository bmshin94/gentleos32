/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: 2048.c - 2048 game
 */

#include <gui.h>

enum {
    LINE_SIZE = 4,

    GRID_ROWS = LINE_SIZE,
    GRID_COLS = LINE_SIZE,
    GRID_CELL_WIDTH = 48,
    GRID_CELL_HEIGHT = 48,
    GRID_BORDER = 1,
    GRID_WIDTH = GRID_WIDTH_SPACED(GRID_CELL_WIDTH, GRID_COLS, GRID_BORDER),
    GRID_HEIGHT = GRID_HEIGHT_SPACED(GRID_CELL_HEIGHT, GRID_ROWS, GRID_BORDER),
    GRID_X = 0,
    GRID_Y = TITLE_BAR_HEIGHT - 1,

    WINDOW_WIDTH = GRID_X + GRID_WIDTH,
    WINDOW_HEIGHT = GRID_Y + GRID_HEIGHT,

    CELL_EXP_WIN = 11, /* 2^11 == 2048 */
    FLASH_TICKS = TICK_FREQUENCY * 15 / 100, /* 0.15s */
};

typedef struct {
    uint8_t exp;
    uint8_t flashed;
} cell_st;

typedef struct {
    uint8_t window_pixels[WINDOW_WIDTH * WINDOW_HEIGHT];
    surface_st window_surface;
    window_st window;

    widget_st title_bar;
    widget_st close_button;
    widget_st *widgets[2];

    grid_st grid;

    cell_st board[GRID_COLS][GRID_ROWS];

    uint32_t score;
    int game_over;
    int game_won;
    int skip_intro;

    unsigned flash_ticks;
} app_state_st;

static uint32_t best_score;
static app_state_st *app_state = NULL;

static void
update_status(void)
{
    static const char *restart = "R: Restart";
    app_state_st *a = app_state;

    if (a->game_won) {
        gui_status_set("You won!  Score: %u  Best: %u  \xb3  %s",
            a->score, best_score, restart);
    } else if (a->game_over) {
        gui_status_set("Game over!  Score: %u  Best: %u  \xb3  %s",
            a->score, best_score, restart);
    } else {
        gui_status_set("Score: %u  Best: %u  \xb3  %s",
            a->score, best_score, restart);
    }
}

static void
add_score(uint32_t score)
{
    app_state_st *a = app_state;

    a->score += score;

    if (a->score > best_score) {
        best_score = a->score;
    }
}

/* Check for an empty cell or two equal neighbours */
static int
has_moves(void)
{
    app_state_st *a = app_state;
    uint8_t cur;
    int col, row;

    for (row = 0; row < GRID_ROWS; ++row) {
        for (col = 0; col < GRID_COLS; ++col) {
            cur = a->board[col][row].exp;

            if (!cur) {
                return 1;
            }

            if (col + 1 < GRID_COLS && a->board[col + 1][row].exp == cur) {
                return 1;
            }

            if (row + 1 < GRID_ROWS && a->board[col][row + 1].exp == cur) {
                return 1;
            }
        }
    }

    return 0;
}

static int
has_won(void)
{
    app_state_st *a = app_state;
    int col, row;

    for (row = 0; row < GRID_ROWS; ++row) {
        for (col = 0; col < GRID_COLS; ++col) {
            if (a->board[col][row].exp >= CELL_EXP_WIN) {
                return 1;
            }
        }
    }

    return 0;
}

static int
count_empty_cells(void)
{
    app_state_st *a = app_state;
    int col, row;
    int ret = 0;

    for (row = 0; row < GRID_ROWS; ++row) {
        for (col = 0; col < GRID_COLS; ++col) {
            if (!a->board[col][row].exp) {
                ++ret;
            }
        }
    }

    return ret;
}

static void
draw_cell(int col, int row)
{
    app_state_st *a = app_state;
    cell_st *cell = &a->board[col][row];
    rect_st rect = gui_grid_cell_rect(&a->grid, col, row);
    uint8_t bg, fg;
    char str[8];

    if (cell->flashed) {
        bg = COLOR_TITLE_ACT_BG;
        fg = COLOR_TITLE_ACT_FG;
    } else if (cell->exp) {
        bg = COLOR_CARD_FRONT_BG;
        fg = COLOR_CARD_BLACK_FG;
    } else {
        bg = COLOR_WIDGET_BG;
        fg = COLOR_WIDGET_FG;
    }

    gui_surface_draw_rect(a->window.surface, rect, bg);

    if (cell->exp) {
        snprintf(str, sizeof(str), "%u", 1u << cell->exp);
        gui_surface_draw_str_cc(a->window.surface, rect, font_8x16, str, fg, bg);
    }

    gui_wm_render_window_region(&a->window, rect);
}

static void
draw_board(void)
{
    int col, row;

    for (row = 0; row < GRID_ROWS; ++row) {
        for (col = 0; col < GRID_COLS; ++col) {
            draw_cell(col, row);
        }
    }
}

static void
clear_flash(void)
{
    app_state_st *a = app_state;
    cell_st *cell;
    int col, row;

    a->flash_ticks = 0;

    for (row = 0; row < GRID_ROWS; ++row) {
        for (col = 0; col < GRID_COLS; ++col) {
            cell = &a->board[col][row];

            if (cell->flashed) {
                cell->flashed = 0;
                draw_cell(col, row);
            }
        }
    }
}

static void
clear_board(void)
{
    app_state_st *a = app_state;
    int col, row;

    for (row = 0; row < GRID_ROWS; ++row) {
        for (col = 0; col < GRID_COLS; ++col) {
            a->board[col][row].exp = 0;
            a->board[col][row].flashed = 0;
        }
    }

    draw_board();
}

static void
set_random_cell(void)
{
    app_state_st *a = app_state;
    int col, row, empty_index, empty_count;

    empty_count = count_empty_cells();

    if (!empty_count) {
        return;
    }

    empty_index = rand() % empty_count;

    for (row = 0; row < GRID_ROWS; ++row) {
        for (col = 0; col < GRID_COLS; ++col) {
            if (a->board[col][row].exp) {
                continue;
            }

            if (empty_index > 0) {
                --empty_index;
                continue;
            }

            /* 1 in 10 times start with 4 */
            a->board[col][row].exp = (rand() % 10) ? 1 : 2;
            draw_cell(col, row);

            return;
        }
    }
}

/* Shift back all cells after given index */
static int
shift_line(cell_st *line, int i)
{
    int shifted = 0;

    for (; i + 1 < LINE_SIZE; ++i) {
        line[i] = line[i + 1];

        if (line[i].exp) {
            shifted = 1;
        }
    }

    line[i].exp = 0;
    line[i].flashed = 0;

    return shifted;
}

/* Slide all cells towards the start of the line, merging adjacent equal cells */
static void
slide_line(cell_st *line)
{
    int i = 0;
    int shifted;
    cell_st *cur, *next;

    while (i < LINE_SIZE - 1) {
        cur = &line[i];
        next = &line[i + 1];

        if (!cur->exp) {
            /* Empty current: shift the subsequent ones and check current again */
            shifted = shift_line(line, i);

            if (!shifted) {
                return;
            }
        } else if (!next->exp) {
            /* Empty next: shift the subsequent ones and check current again */
            shifted = shift_line(line, i + 1);

            if (!shifted) {
                return;
            }
        } else if (cur->exp == next->exp) {
            /* Equal next: merge current, set next to 0, advance to next */
            ++cur->exp;
            cur->flashed = 1;
            next->exp = 0;

            add_score(1u << cur->exp);

            ++i;
        } else {
            /* Non-equal next: advance to next */
            ++i;
        }
    }
}

static int
slide_board(int dc, int dr)
{
    app_state_st *a = app_state;
    cell_st line[LINE_SIZE];
    cell_st *new_cell, *old_cell;
    int i, j, col, row, start_col, start_row;
    int changed = 0;

    for (i = 0; i < LINE_SIZE; ++i) {
        start_col = dc ? ((dc > 0) ? LINE_SIZE - 1 : 0) : i;
        start_row = dr ? ((dr > 0) ? LINE_SIZE - 1 : 0) : i;

        for (j = 0; j < LINE_SIZE; ++j) {
            col = start_col - j * dc;
            row = start_row - j * dr;

            line[j] = a->board[col][row];
        }

        slide_line(line);

        for (j = 0; j < LINE_SIZE; ++j) {
            col = start_col - j * dc;
            row = start_row - j * dr;
            old_cell = &a->board[col][row];
            new_cell = &line[j];

            if (new_cell->exp != old_cell->exp || new_cell->flashed) {
                old_cell->exp = new_cell->exp;
                old_cell->flashed = new_cell->flashed;
                draw_cell(col, row);
                changed = 1;
            }
        }
    }

    return changed;
}

static void
restart_game(void)
{
    app_state_st *a = app_state;

    a->game_over = 0;
    a->game_won = 0;
    a->score = 0;
    a->flash_ticks = 0;

    clear_board();
    set_random_cell();
    set_random_cell();

    update_status();
}

static void
make_move(int col_step, int row_step)
{
    app_state_st *a = app_state;

    clear_flash();

    if (!slide_board(col_step, row_step)) {
        return;
    }

    a->flash_ticks = FLASH_TICKS;

    set_random_cell();

    a->game_won = has_won();
    a->game_over = !has_moves();

    update_status();
}

static void
draw_window(window_st *window)
{
    gui_window_draw(window, COLOR_BORDER);
    draw_board();
}

static void
on_tick(window_st *window _unsd)
{
    app_state_st *a = app_state;

    if (!a->flash_ticks) {
        return;
    }

    --a->flash_ticks;

    if (!a->flash_ticks) {
        clear_flash();
    }
}

static void
on_key_down(window_st *window _unsd, event_st event)
{
    app_state_st *a = app_state;

    if (event.key_code == KEY_R) {
        restart_game();
        return;
    }

    if (a->game_over || a->game_won) {
        return;
    }

    switch (event.key_code) {
    case KEY_LEFT: make_move(-1, 0); break;
    case KEY_RIGHT: make_move(1, 0); break;
    case KEY_UP: make_move(0, -1); break;
    case KEY_DOWN: make_move(0, 1); break;
    default: break;
    }
}

static void
on_active_change(window_st *window)
{
    app_state_st *a = app_state;

    if (!window->active) {
        return;
    }

    if (!a->skip_intro) {
        gui_status_set("Use arrow keys to slide and merge tiles until you reach 2048");
        a->skip_intro = 1;
        return;
    }

    update_status();
}

static void
close_window(window_st *window _unsd)
{
    gui_wm_remove_window(window);
    app_2048.main_window = NULL;

    heap_free(app_state);
    app_state = NULL;
}

static void
init_window(void)
{
    app_state_st *a = app_state;

    a->window_surface.size.width = WINDOW_WIDTH;
    a->window_surface.size.height = WINDOW_HEIGHT;
    a->window_surface.pitch = WINDOW_WIDTH;
    a->window_surface.pixels = a->window_pixels;

    a->window.surface = &a->window_surface;
    a->window.title = "2048";
    a->window.widgets = a->widgets;
    a->window.widgets_capacity = sizeof(a->widgets) / sizeof(a->widgets[0]);
    a->window.draw = draw_window;
    a->window.on_key_down = on_key_down;
    a->window.on_active_change = on_active_change;
    a->window.on_tick = on_tick;
    a->window.on_close = close_window;

    gui_window_init_frame(&a->window, &a->title_bar, &a->close_button);
}

static void
init_grid(void)
{
    app_state_st *a = app_state;

    a->grid.cell_width = GRID_CELL_WIDTH;
    a->grid.cell_height = GRID_CELL_HEIGHT;
    a->grid.cols = GRID_COLS;
    a->grid.rows = GRID_ROWS;
    a->grid.border = GRID_BORDER;
    a->grid.x = GRID_X;
    a->grid.y = GRID_Y;
}

static int
init_app(void)
{
    ASSERT(!app_state);

    app_state = heap_alloc(sizeof(app_state_st), "2048 app", 0);

    if (!app_state) {
        return E_NOT_ENOUGH_MEMORY;
    }

    init_window();
    init_grid();
    restart_game();

    app_2048.main_window = &app_state->window;

    return E_OK;
}

global app_st app_2048 = {
    .icon = &icon_2048,
    .init = init_app,
};
