// fe.c -- the front end of Simon Tatham's Puzzles on c4: everything but
// the display and the clock, which are the platform's (c4fe.h). The same
// unit for C4IX, the bare machine, C4DOS and the gcc build.
//
// The game draws through drawing_api into an 8-bit framebuffer of the
// platform's size, here and not on the host's canvas: so what it drew can
// be checked, and is the same bytes whichever compiler built it. Colours
// are the game's own numbers, and the palette goes along with each frame.
// All of this is integer arithmetic, text too: a stroke font, which any
// size draws (font.h).
//
// A row of buttons above the puzzle and a status line below it; the mouse
// and the keyboard go to the midend as they do in the other front ends.
// Where the window has a menu bar to give (C4IX's desktop), Game, Type and
// Help menus too: the buttons' actions, the game's presets, and a line
// about it.
//
//   NAME [--window|--fullscreen] [ID]
//                              play: ID is a game ID or a size, as
//                              the other front ends take (`net 7x7`);
//                              in a window, or on the whole display
//                              (C4IX: a window where it was started
//                              in one, by default)
//   NAME --test N [ID]         make the game, play N scripted steps, solve
//                              it, printing a checksum of what was drawn
//                              (make test-puzzles)

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "puzzles.h"
#include "c4fe.h"
#include "font.h"

enum {
    BAR_H = 22, STAT_H = 18,
    // the front end's own colours, past any game's
    UI_BAR = 250, UI_INK, UI_LIGHT, UI_DARK, UI_STATUS,
    MAX_PRESETS = 64, MAX_BUTTONS = 8
};

struct frontend {
    midend *me;
    drawing *dr;
    unsigned char *fb;
    int w, h;                   // the framebuffer
    int bw, bh, ox, oy;         // the puzzle in it
    int pal[256];
    int cx0, cy0, cx1, cy1;     // the clip, in the framebuffer, cx1 and cy1 past it
    int dirty;
    int timer_on, last_ms;
    int held;                   // the mouse button down in the puzzle, or -1
    int quit;
    int seed_fixed;
    int fill;                   // in a window: the puzzle as big as it holds
    int npresets, preset;
    int pids[MAX_PRESETS];      // each preset's id, as midend_which_preset gives it
    int game_acts[12];          // the Game menu's items: a button each, or -1
    game_params *pparams[MAX_PRESETS];
    char *ptitles[MAX_PRESETS];
    char *status;               // the game's status line
    char *message;              // or, until the next move, ours
};
static struct frontend fe0;

struct blitter { int w, h; unsigned char *pix; };

void fatal(const char *fmt, ...)
{
    va_list ap;
    printf("fatal: ");
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    printf("\n");
    exit(1);
}
#ifdef DEBUGGING
void debug_printf(const char *fmt, ...) { }
#endif
void document_add_puzzle(document *doc, const game *game, game_params *par,
                         game_ui *ui, game_state *st, game_state *st2) { }

void get_random_seed(void **randseed, int *randseedsize)
{
    int *s = snewn(2, int);
    s[0] = fe0.seed_fixed ? 4 : plat_ms();
    s[1] = fe0.seed_fixed ? 8 : plat_ticks();
    *randseed = s;
    *randseedsize = 2 * sizeof(int);
}
void frontend_default_colour(frontend *fe, float *output)
{
    output[0] = output[1] = output[2] = 0.85F;
}
void activate_timer(frontend *fe)
{
    if (!fe->timer_on) fe->last_ms = plat_ms();
    fe->timer_on = 1;
}
void deactivate_timer(frontend *fe) { fe->timer_on = 0; }

// ---- pixels ----

static void hspan(frontend *fe, int x0, int x1, int y, int col)
{
    if (y < fe->cy0 || y >= fe->cy1) return;
    if (x0 < fe->cx0) x0 = fe->cx0;
    if (x1 >= fe->cx1) x1 = fe->cx1 - 1;
    if (x0 <= x1) memset(fe->fb + y * fe->w + x0, col, x1 - x0 + 1);
}
static void fill(frontend *fe, int x, int y, int w, int h, int col)
{
    int y1 = y + h;
    if (y < fe->cy0) y = fe->cy0;
    if (y1 > fe->cy1) y1 = fe->cy1;
    for (; y < y1; y++) hspan(fe, x, x + w - 1, y, col);
}
static void plot(frontend *fe, int x, int y, int col)
{
    if (x >= fe->cx0 && x < fe->cx1 && y >= fe->cy0 && y < fe->cy1)
        fe->fb[y * fe->w + x] = col;
}
// a line, in a square pen t wide
static void line(frontend *fe, int x0, int y0, int x1, int y1, int t, int col)
{
    int dx = x1 > x0 ? x1 - x0 : x0 - x1, sx = x1 > x0 ? 1 : -1;
    int dy = y1 > y0 ? y1 - y0 : y0 - y1, sy = y1 > y0 ? 1 : -1;
    int err = dx - dy, e2, back = t / 2;
    if (t == 1 && dy == 0) {
        hspan(fe, x0 < x1 ? x0 : x1, x0 < x1 ? x1 : x0, y0, col);
        return;
    }
    while (1) {
        if (t == 1) plot(fe, x0, y0, col);
        else fill(fe, x0 - back, y0 - back, t, t, col);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
}

// ---- text: font.h's strokes, a glyph 8 units to the size ----

static void stroke(frontend *fe, int x, int y, int size, int t, const char *s, int col)
{
    int have = 0, px = 0, py = 0, qx, qy;
    for (; *s; ) {
        if (*s == ' ') { have = 0; s++; continue; }
        qx = x + (s[0] - '0') * size / 8;
        qy = y + (s[1] - '0') * size / 8;
        if (have) line(fe, px, py, qx, qy, t, col);
        else if (!s[2] || s[2] == ' ') fill(fe, qx - t / 2, qy - t / 2, t, t, col);
        px = qx; py = qy; have = 1;
        s += 2;
    }
}
static void text(frontend *fe, int x, int y, int size, int align, int col, const char *str)
{
    int adv = 11 * size / 16, t = size / 10;
    int n = strlen(str), i, c;
    if (t < 1) t = 1;
    if (adv < 5) adv = 5;
    if (align & ALIGN_HCENTRE) x -= (n * adv - adv / 3) / 2;
    else if (align & ALIGN_HRIGHT) x -= n * adv - adv / 3;
    // y names the baseline, or the middle of a capital
    y -= 6 * size / 8;
    if (align & ALIGN_VCENTRE) y += 3 * size / 8;
    for (i = 0; i < n; i++, x += adv) {
        c = (unsigned char)str[i];
        if (c > ' ' && c < 127) stroke(fe, x, y, size, t, font_strokes[c - 33], col);
    }
}

// ---- drawing_api: what the game calls, the puzzle's origin added ----

static void c4_draw_text(drawing *dr, int x, int y, int fonttype, int fontsize,
                         int align, int colour, const char *str)
{
    frontend *fe = GET_HANDLE_AS_TYPE(dr, frontend);
    text(fe, x + fe->ox, y + fe->oy, fontsize, align, colour, str);
}
static void c4_draw_rect(drawing *dr, int x, int y, int w, int h, int colour)
{
    frontend *fe = GET_HANDLE_AS_TYPE(dr, frontend);
    fill(fe, x + fe->ox, y + fe->oy, w, h, colour);
}
static void c4_draw_line(drawing *dr, int x1, int y1, int x2, int y2, int colour)
{
    frontend *fe = GET_HANDLE_AS_TYPE(dr, frontend);
    line(fe, x1 + fe->ox, y1 + fe->oy, x2 + fe->ox, y2 + fe->oy, 1, colour);
}
static void c4_draw_circle(drawing *dr, int cx, int cy, int radius,
                           int fillcolour, int outlinecolour)
{
    frontend *fe = GET_HANDLE_AS_TYPE(dr, frontend);
    int x, y, last, r2 = radius * radius + radius;
    cx += fe->ox;
    cy += fe->oy;
    // each row, from the widest out: the outline meets the last row's end
    x = radius;
    last = radius;
    for (y = 0; y <= radius; y++) {
        while (x > 0 && x * x + y * y > r2) x--;
        if (fillcolour >= 0) {
            hspan(fe, cx - x, cx + x, cy + y, fillcolour);
            hspan(fe, cx - x, cx + x, cy - y, fillcolour);
        }
        if (outlinecolour >= 0) {
            int e = last > x ? last - 1 : x;
            hspan(fe, cx - e, cx - x, cy + y, outlinecolour);
            hspan(fe, cx - e, cx - x, cy - y, outlinecolour);
            hspan(fe, cx + x, cx + e, cy + y, outlinecolour);
            hspan(fe, cx + x, cx + e, cy - y, outlinecolour);
        }
        last = x;
    }
    if (outlinecolour >= 0 && last > 0) {
        hspan(fe, cx - last, cx + last, cy + radius, outlinecolour);
        hspan(fe, cx - last, cx + last, cy - radius, outlinecolour);
    }
}
static void c4_draw_update(drawing *dr, int x, int y, int w, int h)
{
    frontend *fe = GET_HANDLE_AS_TYPE(dr, frontend);
    fe->dirty = 1;
}
static void clip_all(frontend *fe)
{
    fe->cx0 = 0; fe->cy0 = 0; fe->cx1 = fe->w; fe->cy1 = fe->h;
}
static void c4_clip(drawing *dr, int x, int y, int w, int h)
{
    frontend *fe = GET_HANDLE_AS_TYPE(dr, frontend);
    x += fe->ox;
    y += fe->oy;
    fe->cx0 = x < 0 ? 0 : x;
    fe->cy0 = y < 0 ? 0 : y;
    fe->cx1 = x + w > fe->w ? fe->w : x + w;
    fe->cy1 = y + h > fe->h ? fe->h : y + h;
}
static void c4_unclip(drawing *dr)
{
    clip_all(GET_HANDLE_AS_TYPE(dr, frontend));
}
static void c4_start_draw(drawing *dr) { }
static void c4_end_draw(drawing *dr) { }
static void draw_status(frontend *fe)
{
    int y = fe->h - STAT_H;
    clip_all(fe);
    fill(fe, 0, y, fe->w, STAT_H, UI_STATUS);
    line(fe, 0, y, fe->w - 1, y, 1, UI_DARK);
    text(fe, 6, y + STAT_H / 2 + 1, 12, ALIGN_VCENTRE, UI_INK,
         fe->message ? fe->message : fe->status ? fe->status : "");
    fe->dirty = 1;
}
static void c4_status_bar(drawing *dr, const char *str)
{
    frontend *fe = GET_HANDLE_AS_TYPE(dr, frontend);
    int cx0 = fe->cx0, cy0 = fe->cy0, cx1 = fe->cx1, cy1 = fe->cy1;
    sfree(fe->status);
    fe->status = dupstr(str);
    draw_status(fe);
    fe->cx0 = cx0; fe->cy0 = cy0; fe->cx1 = cx1; fe->cy1 = cy1;
}
static blitter *c4_blitter_new(drawing *dr, int w, int h)
{
    blitter *bl = snew(blitter);
    bl->w = w;
    bl->h = h;
    bl->pix = snewn(w * h, unsigned char);
    return bl;
}
static void c4_blitter_free(drawing *dr, blitter *bl)
{
    sfree(bl->pix);
    sfree(bl);
}
// a saved rectangle may stick out of the framebuffer: those pixels are
// neither saved nor drawn
static void c4_blitter_save(drawing *dr, blitter *bl, int x, int y)
{
    frontend *fe = GET_HANDLE_AS_TYPE(dr, frontend);
    int r, c, fx, fy;
    for (r = 0; r < bl->h; r++) {
        fy = y + fe->oy + r;
        for (c = 0; c < bl->w; c++) {
            fx = x + fe->ox + c;
            bl->pix[r * bl->w + c] = (fx >= 0 && fx < fe->w && fy >= 0 && fy < fe->h)
                ? fe->fb[fy * fe->w + fx] : 0;
        }
    }
}
static void c4_blitter_load(drawing *dr, blitter *bl, int x, int y)
{
    frontend *fe = GET_HANDLE_AS_TYPE(dr, frontend);
    int r, c;
    for (r = 0; r < bl->h; r++)
        for (c = 0; c < bl->w; c++)
            plot(fe, x + fe->ox + c, y + fe->oy + r, bl->pix[r * bl->w + c]);
}
static const drawing_api c4_drawing = {
    .version = 1,
    .draw_text = c4_draw_text,
    .draw_rect = c4_draw_rect,
    .draw_line = c4_draw_line,
    .draw_polygon = draw_polygon_fallback,
    .draw_circle = c4_draw_circle,
    .draw_update = c4_draw_update,
    .clip = c4_clip,
    .unclip = c4_unclip,
    .start_draw = c4_start_draw,
    .end_draw = c4_end_draw,
    .status_bar = c4_status_bar,
    .blitter_new = c4_blitter_new,
    .blitter_free = c4_blitter_free,
    .blitter_save = c4_blitter_save,
    .blitter_load = c4_blitter_load,
};

// ---- the buttons ----

static const char *const button_names[] = {
    "New", "Restart", "Undo", "Redo", "Solve", "Type", "Quit"
};
#define NBUTTONS 7
static int button_x(frontend *fe, int i) { return 4 + i * (fe->w - 8) / NBUTTONS; }
static void draw_bar(frontend *fe)
{
    int i, x, w;
    clip_all(fe);
    fill(fe, 0, 0, fe->w, BAR_H, UI_BAR);
    for (i = 0; i < NBUTTONS; i++) {
        x = button_x(fe, i);
        w = button_x(fe, i + 1) - x - 2;
        fill(fe, x, 2, w, BAR_H - 5, UI_LIGHT);
        line(fe, x, BAR_H - 3, x + w - 1, BAR_H - 3, 1, UI_DARK);
        line(fe, x + w - 1, 2, x + w - 1, BAR_H - 3, 1, UI_DARK);
        text(fe, x + w / 2, BAR_H / 2 - 1, 12, ALIGN_HCENTRE | ALIGN_VCENTRE, UI_INK, button_names[i]);
    }
    fe->dirty = 1;
}

// ---- the game ----

static void add_presets(frontend *fe, struct preset_menu *m)
{
    int i;
    for (i = 0; i < m->n_entries; i++) {
        if (m->entries[i].submenu) add_presets(fe, m->entries[i].submenu);
        else if (fe->npresets < MAX_PRESETS) {
            fe->pparams[fe->npresets] = m->entries[i].params;
            fe->ptitles[fe->npresets] = m->entries[i].title;
            fe->pids[fe->npresets] = m->entries[i].id;
            fe->npresets++;
        }
    }
}
static int ui8(float f)
{
    int v = (int)(f * 255.0F + 0.5F);
    return v < 0 ? 0 : v > 255 ? 255 : v;
}
// the puzzle's size and place, for its parameters; everything drawn again
static void layout(frontend *fe)
{
    float *c;
    int i, n, x = fe->w, y = fe->h - BAR_H - STAT_H;
    midend_size(fe->me, &x, &y, false, 1.0);
    // playing, up to twice the game's own size, as the display has room: a
    // puzzle has no frame rate to keep. In a window, which may be any size,
    // as big as it holds. The test draws at its own size.
    if (!fe->seed_fixed) {
        int ax = fe->w, ay = fe->h - BAR_H - STAT_H;
        x = fe->fill || 2 * x > ax ? ax : 2 * x;
        y = fe->fill || 2 * y > ay ? ay : 2 * y;
        midend_size(fe->me, &x, &y, true, 1.0);
    }
    fe->bw = x;
    fe->bh = y;
    // on a word: test_sum takes the rows a word at a time
    fe->ox = ((fe->w - x) / 2) & ~3;
    fe->oy = BAR_H + (fe->h - BAR_H - STAT_H - y) / 2;
    c = midend_colours(fe->me, &n);
    if (n > UI_BAR) fatal("%d colours", n);
    for (i = 0; i < n; i++)
        fe->pal[i] = (ui8(c[i * 3]) << 16) | (ui8(c[i * 3 + 1]) << 8) | ui8(c[i * 3 + 2]);
    sfree(c);
    fe->pal[UI_BAR] = 0xc0c0c0;
    fe->pal[UI_INK] = 0x000000;
    fe->pal[UI_LIGHT] = 0xe0e0e0;
    fe->pal[UI_DARK] = 0x606060;
    fe->pal[UI_STATUS] = 0xd8d8d8;
    clip_all(fe);
    memset(fe->fb, 0, fe->w * fe->h);
    draw_bar(fe);
    draw_status(fe);
    midend_force_redraw(fe->me);
    fe->dirty = 1;
}
// the window's new size: a frame that size, and the puzzle laid out in it again
static void resize(frontend *fe, int w, int h)
{
    if (w == fe->w && h == fe->h) return;
    sfree(fe->fb);
    fe->w = w;
    fe->h = h;
    fe->fb = snewn(w * h, unsigned char);
    layout(fe);
}
static void set_message(frontend *fe, const char *m)
{
    fe->message = (char *)m;
    draw_status(fe);
}
static void new_game(frontend *fe)
{
    midend_new_game(fe->me);
    // the game's status line stays as it is: drawing.c passes the game's
    // text on only when it changes, so one let go of here would stay blank
    // whenever the new game's first is the old game's last (Net's "Active:
    // 1/25" twice running)
    set_message(fe, NULL);
    layout(fe);
}
static void press(frontend *fe, int x, int y, int button)
{
    if (fe->message) set_message(fe, NULL);
    if (midend_process_key(fe->me, x, y, button) == PKR_QUIT) fe->quit = 1;
}
static void send_menus(frontend *fe);
static void do_button(frontend *fe, int i)
{
    const char *err;
    switch (i) {
      case 0: new_game(fe); break;
      case 1: midend_restart_game(fe->me); break;
      case 2: press(fe, 0, 0, UI_UNDO); break;
      case 3: press(fe, 0, 0, UI_REDO); break;
      case 4:
        err = midend_solve(fe->me);
        if (err) set_message(fe, err);
        break;
      case 5:
        if (fe->npresets) {
            fe->preset = (fe->preset + 1) % fe->npresets;
            midend_set_params(fe->me, fe->pparams[fe->preset]);
            new_game(fe);
            set_message(fe, fe->ptitles[fe->preset]);
            send_menus(fe);
        }
        break;
      case 6: fe->quit = 1; break;
    }
}

// ---- the menu bar ----

static void game_item(frontend *fe, int *n, const char *label, int button)
{
    plat_item(0, label);
    fe->game_acts[(*n)++] = button;
}
// Game: the buttons' actions, with the midend's keys for them; Type: the
// presets, the one in play checked; Help: what this is. Sent again when the
// preset changes.
static void send_menus(frontend *fe)
{
    char item[64];
    int i, n = 0, cur = midend_which_preset(fe->me);
    plat_menu(0, "Game");
    game_item(fe, &n, "New   N", 0);
    game_item(fe, &n, "Restart", 1);
    game_item(fe, &n, "-", -1);
    game_item(fe, &n, "Undo   U", 2);
    game_item(fe, &n, "Redo   R", 3);
    if (thegame.can_solve) game_item(fe, &n, "Solve   Ctrl+S", 4);
    game_item(fe, &n, "-", -1);
    game_item(fe, &n, "Exit   Q", 6);
    if (fe->npresets) {
        plat_menu(1, "Type");
        for (i = 0; i < fe->npresets; i++) {
            if (fe->pids[i] == cur) fe->preset = i;
            snprintf(item, sizeof(item), "%s%s", fe->pids[i] == cur ? "*" : "", fe->ptitles[i]);
            plat_item(1, item);
        }
    }
    plat_menu(fe->npresets ? 2 : 1, "Help");
    plat_item(fe->npresets ? 2 : 1, "About");
}
static void menu_pick(frontend *fe, int menu, int item)
{
    static char about[96];
    if (menu == 0) {
        if (item >= 0 && item < 12 && fe->game_acts[item] >= 0) do_button(fe, fe->game_acts[item]);
    } else if (menu == 1 && fe->npresets) {
        if (item >= 0 && item < fe->npresets) {
            fe->preset = item;
            midend_set_params(fe->me, fe->pparams[item]);
            new_game(fe);
            set_message(fe, fe->ptitles[item]);
            send_menus(fe);
        }
    } else {
        snprintf(about, sizeof(about), "%s, from Simon Tatham's Portable Puzzle Collection", thegame.name);
        set_message(fe, about);
    }
}
static void key(frontend *fe, int code, int ch, int mods)
{
    int b = 0, m = 0;
    if (mods & FE_SHIFT) m |= MOD_SHFT;
    if (mods & FE_CTRL) m |= MOD_CTRL;
    switch (code) {
      case 37: b = CURSOR_LEFT | m; break;
      case 38: b = CURSOR_UP | m; break;
      case 39: b = CURSOR_RIGHT | m; break;
      case 40: b = CURSOR_DOWN | m; break;
      case 13: b = CURSOR_SELECT; break;
      case 8: case 46: b = '\177'; break;
      case 27: fe->quit = 1; return;
      default:
        if (ch == ' ') b = CURSOR_SELECT2;
        // Ctrl-Z and the rest, as the midend knows them
        else if (mods & FE_CTRL) b = (code >= 'A' && code <= 'Z') ? code - 'A' + 1 : 0;
        else b = ch;
    }
    if (b) press(fe, 0, 0, b);
}
static void event(frontend *fe, int *ev)
{
    int x = ev[1], y = ev[2];
    switch (ev[0]) {
      case FE_EV_DOWN:
        if (y < BAR_H) {
            int i;
            for (i = 0; i < NBUTTONS; i++)
                if (x >= button_x(fe, i) && x < button_x(fe, i + 1)) do_button(fe, i);
        } else if (fe->held < 0 && ev[3] <= 2) {
            fe->held = ev[3];
            press(fe, x - fe->ox, y - fe->oy, LEFT_BUTTON + fe->held);
        }
        break;
      case FE_EV_UP:
        if (fe->held >= 0 && ev[3] == fe->held) {
            press(fe, x - fe->ox, y - fe->oy, LEFT_RELEASE + fe->held);
            fe->held = -1;
        }
        break;
      case FE_EV_MOVE:
        if (fe->held >= 0) press(fe, x - fe->ox, y - fe->oy, LEFT_DRAG + fe->held);
        break;
      case FE_EV_KEY:
        key(fe, ev[1], ev[2], ev[3]);
        break;
      case FE_EV_MENU:
        menu_pick(fe, ev[1], ev[2]);
        break;
      case FE_EV_SIZE:
        resize(fe, ev[1], ev[2]);
        break;
    }
}
static void play(frontend *fe)
{
    int ev[4], now, status = 0, s;
    while (!fe->quit) {
        if (fe->timer_on) {
            now = plat_ms();
            if (now != fe->last_ms) {
                midend_timer(fe->me, (now - fe->last_ms) / 1000.0F);
                fe->last_ms = now;
            }
        }
        while (!fe->quit && plat_event(ev)) event(fe, ev);
        s = midend_status(fe->me);
        if (s != status) {
            status = s;
            if (s > 0) set_message(fe, "Solved!");
            else if (s < 0) set_message(fe, "No moves left: Undo or Restart");
        }
        if (fe->dirty) {
            fe->dirty = 0;
            plat_show(fe->fb, fe->pal);
        }
        plat_wait(fe->timer_on ? 16 : 40);
    }
}

// ---- --test: the run test-puzzles holds gcc's build and c4lc's to ----

static unsigned test_rnd = 12345;
static unsigned test_sum = 2166136261u;
static int test_frames;
static unsigned rnd(unsigned n)
{
    test_rnd = test_rnd * 1103515245u + 12345u;
    return ((test_rnd >> 16) & 32767) % n;
}
static void sum_word(unsigned w)
{
    test_sum = (test_sum ^ w) * 16777619u;
}
// the puzzle's rows, a word at a time
static void sum_frame(frontend *fe)
{
    int y, x, n = fe->bw / 4;
    unsigned *p;
    if (!fe->dirty) return;
    fe->dirty = 0;
    test_frames++;
    for (y = 0; y < fe->bh; y++) {
        p = (unsigned *)(fe->fb + (fe->oy + y) * fe->w + fe->ox);
        for (x = 0; x < n; x++) sum_word(p[x]);
    }
}
// --dump: the rows of the frame, as runs, to find what differs
static int test_dump;
static void dump_frame(frontend *fe)
{
    int y, x, st, c;
    for (y = 0; y < fe->bh; y++) {
        unsigned char *r = fe->fb + (fe->oy + y) * fe->w + fe->ox;
        printf("row %d:", y);
        for (x = 0; x < fe->bw; x = st) {
            c = r[x];
            for (st = x; st < fe->bw && r[st] == c; st++);
            printf(" %d*%d", c, st - x);
        }
        printf("\n");
    }
}
static void sum_bytes(const char *s)
{
    for (; *s; s++) sum_word((unsigned char)*s);
}
// the animations and flashes a move set off: on a clock of its own
static void test_settle(frontend *fe)
{
    int i;
    sum_frame(fe);
    for (i = 0; fe->timer_on && i < 12; i++) {
        midend_timer(fe->me, 0.04F);
        sum_frame(fe);
    }
    midend_stop_anim(fe->me);
    sum_frame(fe);
}
static void test_click(frontend *fe, int button)
{
    int x = rnd(fe->bw), y = rnd(fe->bh), n, i;
    if (midend_process_key(fe->me, x, y, LEFT_BUTTON + button) == PKR_QUIT) return;
    // sometimes a drag
    n = rnd(4) == 0 ? 1 + rnd(4) : 0;
    for (i = 0; i < n; i++) {
        x = rnd(fe->bw);
        y = rnd(fe->bh);
        midend_process_key(fe->me, x, y, LEFT_DRAG + button);
    }
    midend_process_key(fe->me, x, y, LEFT_RELEASE + button);
}
static void test_step(frontend *fe)
{
    static const int cursors[] = { CURSOR_UP, CURSOR_DOWN, CURSOR_LEFT, CURSOR_RIGHT,
                                   CURSOR_SELECT, CURSOR_SELECT2 };
    int k = rnd(20);
    if (k < 9) test_click(fe, 0);
    else if (k < 13) test_click(fe, 2);
    else if (k < 16) midend_process_key(fe->me, 0, 0, cursors[rnd(6)]);
    else if (k < 18) midend_process_key(fe->me, 0, 0, '1' + rnd(9));
    else if (k == 18) midend_process_key(fe->me, 0, 0, UI_UNDO);
    else midend_process_key(fe->me, 0, 0, UI_REDO);
}
static void test_write(void *ctx, const void *buf, int len)
{
    const unsigned char *b = buf;
    int i;
    for (i = 0; i < len; i++) sum_word(b[i]);
}
static void test_run(frontend *fe, int steps, const char *id)
{
    char *s;
    const char *err;
    int i;
    printf("puzzle %s\n", thegame.name);
    if (id) {
        if ((err = midend_game_id(fe->me, id))) fatal("%s", err);
    } else if (fe->npresets) {
        // the smallest game the game has a name for, and a seed of ours
        char *pe = thegame.encode_params(fe->pparams[0], true);
        char *full = snewn(strlen(pe) + 8, char);
        sprintf(full, "%s#c4", pe);
        if ((err = midend_game_id(fe->me, full))) fatal("%s", err);
        sfree(full);
        sfree(pe);
    }
    midend_new_game(fe->me);
    layout(fe);
    s = midend_get_game_id(fe->me);
    printf("puzzle id %s\n", s);
    sfree(s);
    printf("puzzle size %d %d\n", fe->bw, fe->bh);
    test_settle(fe);
    printf("puzzle start %08x\n", test_sum);
    if (test_dump) dump_frame(fe);
    for (i = 1; i <= steps; i++) {
        test_step(fe);
        test_settle(fe);
        if (i % 10 == 0 || i == steps)
            printf("puzzle step %d %08x frames %d status %d\n", i, test_sum, test_frames, midend_status(fe->me));
    }
    if (thegame.can_solve) {
        err = midend_solve(fe->me);
        printf("puzzle solve %s\n", err ? err : "ok");
        test_settle(fe);
        printf("puzzle solved %08x status %d\n", test_sum, midend_status(fe->me));
    }
    if (midend_can_format_as_text_now(fe->me)) {
        s = midend_text_format(fe->me);
        if (s) { sum_bytes(s); sfree(s); }
    }
    midend_serialise(fe->me, test_write, NULL);
    printf("puzzle end %08x\n", test_sum);
}

int main(int argc, char **argv)
{
    frontend *fe = &fe0;
    int steps = -1, window = -1, i, got = 0;
    const char *id = NULL;
    const char *err;
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--test") && i + 1 < argc) steps = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--dump")) test_dump = 1;
        else if (!strcmp(argv[i], "--window")) window = 1;
        else if (!strcmp(argv[i], "--fullscreen")) window = 0;
        else id = argv[i];
    }
    fe->held = -1;
    if (steps >= 0) {
        fe->seed_fixed = 1;
        fe->w = 640;
        fe->h = 480;
    } else if (!(got = plat_open(thegame.name, window, &fe->w, &fe->h))) {
        printf("%s: no display\n", thegame.name);
        return 1;
    }
    fe->fill = got == 2;
    fe->fb = snewn(fe->w * fe->h, unsigned char);
    clip_all(fe);
    fe->me = midend_new(fe, &thegame, &c4_drawing, fe);
    {
        int limit;
        add_presets(fe, midend_get_presets(fe->me, &limit));
    }
    if (steps >= 0) {
        test_run(fe, steps, id);
        return 0;
    }
    if (id && (err = midend_game_id(fe->me, id))) {
        plat_close();
        printf("%s: %s\n", thegame.name, err);
        return 1;
    }
    new_game(fe);
    send_menus(fe);
    play(fe);
    plat_close();
    return 0;
}
