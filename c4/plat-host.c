// plat-host.c -- the gcc build: no display, only `--test`, the reference
// make test-puzzles holds the c4lc build to.
#include "c4fe.h"
int  plat_open(const char *title, int window, int *w, int *h) { return 0; }
void plat_show(unsigned char *fb, int *pal) { }
int  plat_event(int *ev) { return 0; }
void plat_wait(int ms) { }
void plat_close(void) { }
void plat_menu(int n, const char *title) { }
void plat_item(int n, const char *label) { }
int  plat_ms(void) { return 0; }
int  plat_ticks(void) { return 0; }

// C4IX libc's qsort, for -Dqsort=c4_qsort (c4.mk): elements that compare equal
// come out in whatever order a sort leaves them, and Undead's new_game_desc
// makes its monsters from that order. So a game's ID would differ by libc.
void qsort(void *basev, unsigned n0, unsigned size, int (*cmp)(const void *, const void *))
{
    char *base = basev, *a, *b, t;
    int n = n0, gap, i, j;
    unsigned k;
    for (gap = n / 2; gap > 0; gap /= 2) {
        for (i = gap; i < n; i++) {
            for (j = i - gap; j >= 0; j -= gap) {
                a = base + j * size;
                b = base + (j + gap) * size;
                if (cmp(a, b) <= 0) break;
                for (k = 0; k < size; k++) { t = a[k]; a[k] = b[k]; b[k] = t; }
            }
        }
    }
}
