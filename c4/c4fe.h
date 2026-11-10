// c4fe.h -- what fe.c asks of its platform: plat-c4ix.c, plat-bare.c or,
// for the gcc build, plat-host.c
#ifndef C4FE_H
#define C4FE_H 1

// events: ev[0] is the type, and ev[1..3] as libjs's display has them
enum {
    FE_EV_MOVE = 1,     // x, y, buttons
    FE_EV_DOWN = 2,     // x, y, button (0 left, 1 middle, 2 right)
    FE_EV_UP = 3,       // x, y, button
    FE_EV_KEY = 4,      // keycode, character (0 if none), modifiers
    FE_EV_MENU = 5      // menu, item (rules counted): the window's menu bar
};
enum { FE_SHIFT = 1, FE_CTRL = 2, FE_ALT = 4 };

// the display, and how big a framebuffer to draw it from; 0 when there is none.
// window: 1 a window (--window), 0 the whole display (--fullscreen), -1 as
// the platform chooses
int  plat_open(const char *title, int window, int *w, int *h);
// the frame, w x h bytes and 256 colours 0x00RRGGBB
void plat_show(unsigned char *fb, int *pal);
int  plat_event(int *ev);
// wait up to ms for an event
void plat_wait(int ms);
void plat_close(void);
// a menu bar, where the window has one to give (C4IX's desktop): menu n
// (0 the bar anew), and an item of it ("-" a rule, "*label" checked)
void plat_menu(int n, const char *title);
void plat_item(int n, const char *label);
// milliseconds, and a second source of a new game's seed
int  plat_ms(void);
int  plat_ticks(void);

#endif
