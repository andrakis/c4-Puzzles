// plat-c4ix.c -- the puzzles on C4IX. By default the window the game was
// started in (window.h), a Terminal or one the desktop opened for it alone,
// so a puzzle sits beside other windows; with --window, or when something
// that does not give way has the display, that window too. On the console,
// or with --fullscreen, the whole display, which the desktop gives up until
// the game ends (include/display.h). Two buffers in a window, as Tyrian keeps them: the
// desktop reads the one it was given when it next draws, and says so
// (WIN_FRAME); until then a newer frame waits in the other. A new frame goes in
// the one not shown, so the desktop never reads a buffer being changed or let
// go. The window may be any size (win_resizable): each new one is FE_EV_SIZE,
// and the frames are that size from then on.

#include <stdlib.h>
#include <string.h>
#include "gui.h"
#include "display.h"
#include "window.h"
#include "c4fe.h"
#include "c4ix_user.h"

// the smallest frame: a smaller window shows it scaled down
enum { RING = 65536, DW = 640, DH = 480, MIN_W = 240, MIN_H = 140 };

static int windowed, fw, fh, closed;
static int cw = WIN_W, ch = WIN_H;      // the canvas: the mouse is in its pixels
static int *ring;
static char *fbuf[2];
static int fsize[2], fbw[2], fbh[2];    // each buffer: bytes it holds, the frame in it
static int *fpal[2];
static int sent, waiting, pending;

int plat_open(const char *title, int window, int *w, int *h)
{
    int i;
    if (window < 0) window = win_here();
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
            fsize[i] = fw * fh;
        }
        if (!win_open((char *) title)) return 0;
        win_resizable();
        windowed = 1;
    }
    *w = fw;
    *h = fh;
    return windowed ? 2 : 1;
}
static void send_frame(int b)
{
    win_fb(fbuf[b], fbw[b], fbh[b], fbw[b], fpal[b]);
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
    b = 1 - sent;
    if (fsize[b] < fw * fh) {
        free(fbuf[b]);
        fbuf[b] = (char *) malloc(fw * fh);
        fsize[b] = fbuf[b] ? fw * fh : 0;
        if (!fbuf[b]) { pending = 0; return; }
    }
    memcpy(fbuf[b], fb, fw * fh);
    fbw[b] = fw;
    fbh[b] = fh;
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
        if (k == WIN_SIZE) {
            cw = w[0] > 0 ? w[0] : WIN_W;
            ch = w[1] > 0 ? w[1] : WIN_H;
            // the frames that size, or the smallest, scaled down to it
            w[0] = cw < MIN_W ? MIN_W : cw;
            w[1] = ch < MIN_H ? MIN_H : ch;
            if (w[0] == fw && w[1] == fh) continue;
            fw = w[0];
            fh = w[1];
            ev[0] = FE_EV_SIZE;
            ev[1] = fw; ev[2] = fh; ev[3] = 0;
            return 1;
        }
        if (k == WIN_MENU) {
            ev[0] = FE_EV_MENU;
            ev[1] = w[0]; ev[2] = w[1]; ev[3] = 0;
            return 1;
        }
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
// the menu bar of a window of the game's own (window.h); a Terminal and
// the display have none
void plat_menu(int n, const char *title) { if (windowed && !closed) win_menu(n, (char *) title); }
void plat_item(int n, const char *label) { if (windowed && !closed) win_item(n, (char *) label); }
void plat_close(void)
{
    if (windowed) { if (!closed) win_close(); closed = 1; }
    else { gui_detach(); udisplay_give(); }
}
int plat_ms(void) { return __time(); }
int plat_ticks(void) { return gui_ticks(); }
