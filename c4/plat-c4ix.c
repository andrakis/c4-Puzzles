// plat-c4ix.c -- the puzzles on C4IX. By default the whole display, which
// the desktop gives up until the game ends (include/display.h); with
// --window, or when something that does not give way has the display, the
// Terminal window the game was started from (window.h), so a puzzle can sit
// beside other windows. Two buffers in a window, as Tyrian keeps them: the
// desktop reads the one it was given when it next draws, and says so
// (WIN_FRAME); until then a newer frame waits in the other.

#include <stdlib.h>
#include <string.h>
#include "gui.h"
#include "display.h"
#include "window.h"
#include "c4fe.h"
#include "c4ix_user.h"

enum { RING = 65536, DW = 640, DH = 480 };

static int windowed, fw, fh, closed;
static int cw = WIN_W, ch = WIN_H;      // the canvas: the mouse is in its pixels
static int *ring;
static char *fbuf[2];
static int *fpal[2];
static int sent, waiting, pending;

int plat_open(const char *title, int window, int *w, int *h)
{
    int i;
    if (!window && gui_present() && udisplay_take(0)) {
        fw = DW;
        fh = DH;
        ring = (int *) malloc(RING);
        if (!ring || !gui_attach(ring, RING, fw, fh)) { udisplay_give(); return 0; }
        gui_events(GUI_M_KEYS | GUI_M_MOVE | GUI_M_BUTTONS);
    } else {
        fw = WIN_W;
        fh = WIN_H;
        for (i = 0; i < 2; i++) {
            fbuf[i] = (char *) calloc(fw * fh, 1);
            fpal[i] = (int *) calloc(256, sizeof(int));
            if (!fbuf[i] || !fpal[i]) return 0;
        }
        if (!win_open((char *) title)) return 0;
        windowed = 1;
    }
    *w = fw;
    *h = fh;
    return 1;
}
static void send_frame(int b)
{
    win_fb(fbuf[b], fw, fh, fw, fpal[b]);
    win_present();
    sent = b;
    waiting = 1;
}
void plat_show(unsigned char *fb, int *pal)
{
    int b;
    if (!windowed) {
        gui_blit8(0, 0, fw, fh, (char *) fb, fw, pal, fw, fh);
        gui_show();
        return;
    }
    if (closed) return;
    b = waiting ? 1 - sent : sent;
    memcpy(fbuf[b], fb, fw * fh);
    memcpy(fpal[b], pal, 256 * sizeof(int));
    if (waiting) pending = 1;
    else send_frame(b);
}
int plat_event(int *ev)
{
    int w[3], k;
    if (!windowed) {
        while (gui_poll(ev)) {
            if (ev[0] == GUI_EV_KEYDOWN) { ev[0] = FE_EV_KEY; return 1; }
            if (ev[0] == GUI_EV_MOVE || ev[0] == GUI_EV_DOWN || ev[0] == GUI_EV_UP) return 1;
        }
        return 0;
    }
    if (closed) return 0;
    while (1) {
        k = win_poll(w);
        if (k == WIN_NONE) return 0;
        // the window closed: the game ends
        if (k < 0) { closed = 1; exit(0); }
        if (k == WIN_FRAME) {
            waiting = 0;
            if (pending) { pending = 0; send_frame(1 - sent); }
            continue;
        }
        if (k == WIN_SIZE) { cw = w[0]; ch = w[1]; continue; }
        if (k == WIN_KEY) {
            ev[0] = FE_EV_KEY;
            ev[1] = w[0]; ev[2] = w[1]; ev[3] = w[2];
            return 1;
        }
        if (k == WIN_DOWN || k == WIN_UP || k == WIN_MOVE) {
            ev[0] = k == WIN_DOWN ? FE_EV_DOWN : k == WIN_UP ? FE_EV_UP : FE_EV_MOVE;
            ev[1] = cw ? w[0] * fw / cw : w[0];
            ev[2] = ch ? w[1] * fh / ch : w[1];
            ev[3] = w[2];
            return 1;
        }
    }
}
void plat_wait(int ms)
{
    if (windowed) win_sleep(ms);
    else uevwait(ms, gui_events_ring(), 0);
}
void plat_close(void)
{
    if (windowed) { if (!closed) win_close(); closed = 1; }
    else { gui_detach(); udisplay_give(); }
}
int plat_ms(void) { return __time(); }
int plat_ticks(void) { return gui_ticks(); }
