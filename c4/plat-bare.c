// plat-bare.c -- the puzzles on the bare machine and under C4DOS: the display
// is the game's to take. Under grin, C4DOS's graphical shell, the display is
// grin's, and grin publishes a table in C4DOS's slot 25 while it has it
// (c4's docs/grin-plan.md 3.6): then a window of grin's. A grin whose table
// is version 6 or later gives that window what C4IX's desktop gives one
// (plat-c4ix.c): any size, each new one FE_EV_SIZE with the frames that
// size from then on, and a menu bar (c4's docs/grin.md 3.5).

#include <stdlib.h>
#include "gui.h"
#include "c4fe.h"

// the smallest frame: grin stretches it to a smaller window
enum { RING = 65536, DW = 640, DH = 480, MIN_W = 240, MIN_H = 140 };

// grin's table (c4's src/grin/gkit.h): its first nine words
struct grin_table {
    int magic, version, gen;
    void *env;
    void (*hello)(void);
    int  (*fwin_open)(char *title, int w, int h, int flags);
    void (*fwin_frame)(char *pixels, int w, int h, int pitch, int *pal);
    int  (*fwin_poll)(int *ev);
    int  (*fwin_ctl)(int op, int n, char *s);           // version 6
};
// fwin_open's flag for a window of any size, fwin_ctl's ops, fwin_poll's
// events beyond gui.h's (version 6)
enum { FW_RESIZE = 4, FWC_MENU = 1, FWC_ITEM = 2, FW_EV_MENU = 100, FW_EV_SIZE = 101 };
extern int *__c4dos_api;                // bare.c's: C4DOS's table, 0 on the bare machine

static struct grin_table *grin(void)
{
    struct grin_table *t;
    if (!__c4dos_api || __c4dos_api[0] != 4404292 || __c4dos_api[5] < 4) return 0;
    t = (struct grin_table *) __c4dos_api[25];
    return t && t->magic == 1196575054 && t->version >= 2 ? t : 0;
}
static struct grin_table *g;
static int v6;                          // grin's window: any size, and menus
static int fw = DW, fh = DH;
static int *ring;

int plat_open(const char *title, int window, int *w, int *h)
{
    *w = DW;
    *h = DH;
    if ((g = grin()) != 0) {
        v6 = g->version >= 6;
        if (!g->fwin_open((char *) title, DW, DH, v6 ? FW_RESIZE : 0)) return 0;
        // a window of any size: the game fills it (fe.c), at the size grin tells
        return v6 ? 2 : 1;
    }
    if (!gui_present()) return 0;
    ring = (int *) malloc(RING);
    if (!ring || !gui_attach(ring, RING, DW, DH)) return 0;
    gui_events(GUI_M_KEYS | GUI_M_MOVE | GUI_M_BUTTONS);
    return 1;
}
void plat_show(unsigned char *fb, int *pal)
{
    if (g) { g->fwin_frame((char *) fb, fw, fh, fw, pal); return; }
    gui_blit8(0, 0, DW, DH, (char *) fb, DW, pal, DW, DH);
    gui_show();
}
int plat_event(int *ev)
{
    int k;
    while (1) {
        if (g) {
            // the window closed: the game ends, as when it is quit
            if ((k = g->fwin_poll(ev)) < 0) exit(0);
            if (!k) return 0;
            if (ev[0] == FW_EV_MENU) { ev[0] = FE_EV_MENU; return 1; }
            if (ev[0] == FW_EV_SIZE) {
                // the frames that size, or the smallest, which grin stretches
                k = ev[1] < MIN_W ? MIN_W : ev[1];
                ev[2] = ev[2] < MIN_H ? MIN_H : ev[2];
                ev[1] = k;
                if (ev[1] == fw && ev[2] == fh) continue;
                fw = ev[1];
                fh = ev[2];
                ev[0] = FE_EV_SIZE;
                ev[3] = 0;
                return 1;
            }
        } else if (!gui_poll(ev)) return 0;
        if (ev[0] == GUI_EV_KEYDOWN) { ev[0] = FE_EV_KEY; return 1; }
        if (ev[0] == GUI_EV_MOVE || ev[0] == GUI_EV_DOWN || ev[0] == GUI_EV_UP) return 1;
    }
}
void plat_wait(int ms) { __c4_usleep(ms * 1000); }
void plat_close(void) { if (!g) gui_detach(); }
// the menu bar of grin's window (version 6); the display has none
void plat_menu(int n, const char *title) { if (g && v6) g->fwin_ctl(FWC_MENU, n, (char *) title); }
void plat_item(int n, const char *label) { if (g && v6) g->fwin_ctl(FWC_ITEM, n, (char *) label); }
int plat_ms(void) { return __time(); }
int plat_ticks(void) { return g ? 0 : gui_ticks(); }
