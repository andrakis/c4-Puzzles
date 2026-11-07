# Simon Tatham's Puzzles on c4

This is the port's own directory; everything else in the repository is
upstream's (`git log upstream/main..c4` shows what changed). c4's
`games/README.md` says how a game fits in.

| file | what |
|---|---|
| `fe.c` | the front end: `drawing_api` on an 8-bit framebuffer, the buttons, the status line, the mouse and keys, and `--test` |
| `font.h` | a stroke font, written for this port, so text is the same pixels in every build |
| `plat-c4ix.c` | C4IX: the whole display (`udisplay_take`), or `--window`, the Terminal it was started from |
| `plat-bare.c` | the bare machine and C4DOS, and grin's window under grin |
| `plat-host.c` | gcc's build: no display, only `--test`, and C4IX libc's `qsort` |
| `host-math.c` | gcc's build: C4IX libc's `math.c` in place of glibc's |
| `puzzles-list.c` | `puzzles`, which lists the games |
| `stdint.h`, `stdbool.h`, `tgmath.h` | what C4IX's libc does not have. `tgmath.h` is `math.h`, for both compilers, so both compute in double where upstream's unit says `sqrt` |
| `core.txt`, `games.txt` | the units every game links, and the games |

`puzzles-list.c` and `../c4ix.vfs.txt` follow `games.txt`, and were written from
it and upstream's `CMakeLists.txt`: add a game to all three.

## Playing

```
net                 a new game, the default size
net 7x7             a size, as the other front ends take one
net 5x5:85a78...    a game ID
net --window        in the Terminal window it was started from
puzzles             what is here
```

The buttons along the top are New, Restart, Undo, Redo, Solve, Type (the next
of the game's preset sizes) and Quit. The mouse and keys go to the game as in
upstream's other front ends: `n`, `u`, `r` and `q` work in most games, Ctrl-Z and
Ctrl-Y too, and the arrow keys, Enter and Space are its cursor keys where it has
them. Esc quits.

## Why it draws its own pixels

Everything goes through an 8-bit framebuffer rather than the host's canvas
commands. A game takes few colours, and so a frame costs little. And it can be
summed, which is what `make test-puzzles` needs.

## The test

`NAME --test N` runs with no display. It takes the game's first preset and the
seed `c4`, then plays N steps from a generator of its own: clicks, drags, keys,
undo and redo, in the puzzle, on a virtual clock. Then it solves the game. It
prints the game ID, a running checksum of the puzzle's pixels in every frame
drawn, how many frames there were, and at the end a checksum of the saved game.

`make test-puzzles` holds what c4lc's build prints, inside C4IX for every game
and on the bare machine and under C4DOS for a few, to gcc's: `gcc -m32`, with
SSE's arithmetic, each float rounded as the machine's soft float rounds it.
`--dump` prints the first frame's rows, to find what differs.

gcc's build has C4IX libc's `qsort` and `math.c` rather than glibc's, so only the
compiler differs. Both are needed. Undead's generator makes its monsters from the
order of items a sort finds equal. Untangle places a point on exactly a whole
number by way of `cos`, where the two libraries differ in the last bit.

The test draws at the game's own size, and play goes up to twice that, so
the larger sizes of the same drawing code are not in its sums.
`libjs/tests/web-puzzles.mjs` plays Net in a browser, on the display and in a
window.

## What changed in upstream's code

Only `range.c`: its one bitfield became an `unsigned int`, and its function
typedef `reasoning` became a pointer to a function, each with a prototype
spelled out. c4lc has neither construct.
