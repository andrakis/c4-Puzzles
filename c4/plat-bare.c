// plat-bare.c -- the puzzles on the bare machine and under C4DOS: the display
// is the game's to take. Under grin, C4DOS's graphical shell, the display is
// grin's, and grin publishes a table in C4DOS's slot 25 while it has it
// (c4's docs/grin-plan.md 3.6): then a window of grin's.

#include <stdlib.h>
#include "gui.h"
#include "c4fe.h"

enum { RING = 65536, DW = 640, DH = 480 };

// grin's table (c4's src/grin/gkit.h): its first eight words
struct grin_table {
    int magic, version, gen;
    void *env;
    void (*hello)(void);
    int  (*fwin_open)(char *title, int w, int h, int flags);
    void (*fwin_frame)(char *pixels, int w, int h, int pitch, int *pal);
    int  (*fwin_poll)(int *ev);
};
extern int *__c4dos_api;                // bare.c's: C4DOS's table, 0 on the bare machine

static struct grin_table *grin(void)
{
    struct grin_table *t;
    if (!__c4dos_api || __c4dos_api[0] != 4404292 || __c4dos_api[5] < 4) return 0;
    t = (struct grin_table *) __c4dos_api[25];
    return t && t->magic == 1196575054 && t->version >= 2 ? t : 0;
}
static struct grin_table *g;
static int *ring;

int plat_open(const char *title, int window, int *w, int *h)
{
    *w = DW;
    *h = DH;
    if ((g = grin()) != 0) return g->fwin_open((char *) title, DW, DH, 0);
    if (!gui_present()) return 0;
    ring = (int *) malloc(RING);
    if (!ring || !gui_attach(ring, RING, DW, DH)) return 0;
    gui_events(GUI_M_KEYS | GUI_M_MOVE | GUI_M_BUTTONS);
    return 1;
}
void plat_show(unsigned char *fb, int *pal)
{
    if (g) { g->fwin_frame((char *) fb, DW, DH, DW, pal); return; }
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
        } else if (!gui_poll(ev)) return 0;
        if (ev[0] == GUI_EV_KEYDOWN) { ev[0] = FE_EV_KEY; return 1; }
        if (ev[0] == GUI_EV_MOVE || ev[0] == GUI_EV_DOWN || ev[0] == GUI_EV_UP) return 1;
    }
}
void plat_wait(int ms) { __c4_usleep(ms * 1000); }
void plat_close(void) { if (!g) gui_detach(); }
int plat_ms(void) { return __time(); }
int plat_ticks(void) { return g ? 0 : gui_ticks(); }
