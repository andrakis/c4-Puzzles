# c4.mk -- this game's part of c4's Makefile. c4 includes it from
# games/puzzles/c4.mk, its submodule, and runs it from c4's root, with c4's
# tools and variables (games/README.md in c4). Without it, c4 builds
# everything else, and test-puzzles says it was skipped.

# Simon Tatham's Portable Puzzle Collection: the midend and its helpers,
# one unit a game, and the front end (c4/fe.c) with its platform's half
# (c4/plat-*.c, in .puzzles/plat). The same objects for C4IX, the bare
# machine and C4DOS but for that half; build-images.sh links one image a
# game, from c4-images.sh. c4rlink keeps of the core what the game reaches.
PUZ        := games/puzzles
PUZ_CORE   := $(shell cat $(PUZ)/c4/core.txt 2>/dev/null)
PUZ_GAMES  := $(shell cat $(PUZ)/c4/games.txt 2>/dev/null)
PUZ_HDRS   := $(wildcard $(PUZ)/*.h $(PUZ)/c4/*.h) \
              $(wildcard src/c4ix/libc/include/*.h src/c4ix/libc/include/sys/*.h) \
              src/c4ix/include/display.h src/c4ix/include/window.h src/c4ix/include/window.c \
              libjs/guest/gui.h libjs/guest/gui.c
PUZ_CC     := ./c4sp32 -c 400000 src/c4sp/lisp/c4lc.lisp -O $(IMG_G) -mfuse -fno-builtin -c -conforming \
              -I $(PUZ)/c4 -I src/c4ix/libc/include -I src/c4ix/include -I libjs/guest -I $(PUZ)
puzzles-objs: $(PUZ_CORE:%=.puzzles/%.c4o) $(PUZ_GAMES:%=.puzzles/%.c4o) .puzzles/fe.c4o \
              .puzzles/plat/plat-c4ix.c4o .puzzles/plat/plat-bare.c4o .puzzles/plat/puzzles-list.c4o
.puzzles/%.c4o: $(PUZ)/%.c $(PUZ_HDRS) $(C4LC_LISP) .img_g | c4sp32
	@mkdir -p .puzzles/plat
	$(PUZ_CC) $< $@ > /dev/null
.puzzles/fe.c4o: $(PUZ)/c4/fe.c $(PUZ_HDRS) $(C4LC_LISP) .img_g | c4sp32
	@mkdir -p .puzzles/plat
	$(PUZ_CC) $< $@ > /dev/null
.puzzles/plat/%.c4o: $(PUZ)/c4/%.c $(PUZ_HDRS) $(C4LC_LISP) .img_g | c4sp32
	@mkdir -p .puzzles/plat
	$(PUZ_CC) $< $@ > /dev/null
PHONY += puzzles-objs

# ... and by gcc for the host, with no display (c4/plat-host.c), the
# reference test-puzzles holds the c4lc build to: 32-bit, as that is, and
# with SSE's arithmetic, each float rounded as the machine's soft float does.
# It has C4IX libc's qsort and math.c (c4/plat-host.c, c4/host-math.c), not
# glibc's, whose ties and last bits are their own: only the compiler differs.
PUZ_HCC    := gcc -m32 -O1 -std=gnu99 -w -msse2 -mfpmath=sse -fwrapv -fno-builtin -fno-strict-aliasing \
              -Dqsort=c4_qsort -iquote . -I$(PUZ)/c4 -I$(PUZ)
.puzzles/host/%.o: $(PUZ)/%.c $(wildcard $(PUZ)/*.h $(PUZ)/c4/*.h)
	@mkdir -p .puzzles/host
	$(PUZ_HCC) -c $< -o $@
.puzzles/host/%.o: $(PUZ)/c4/%.c $(wildcard $(PUZ)/*.h $(PUZ)/c4/*.h)
	@mkdir -p .puzzles/host
	$(PUZ_HCC) -c $< -o $@
.puzzles/host/bin/%: .puzzles/host/%.o $(PUZ_CORE:%=.puzzles/host/%.o) .puzzles/host/fe.o .puzzles/host/plat-host.o .puzzles/host/host-math.o
	@mkdir -p .puzzles/host/bin
	gcc -m32 -o $@ $^ -lm
.PRECIOUS: .puzzles/host/%.o
puzzles-host: $(PUZ_GAMES:%=.puzzles/host/bin/%)
PHONY += puzzles-host

# test-puzzles: each game, with the seed --test gives it, plays scripted
# moves (c4/fe.c: a generator of our own, clicks, drags and keys in the
# puzzle, every animation on a virtual clock), then solves; what it drew
# and its saved game are summed. gcc's build, and c4lc's inside C4IX, and
# for those in PUZ_TEST_BARE on the bare machine and under C4DOS, must print
# the same.
PUZ_STEPS     := 40
PUZ_TEST_BARE := net fifteen untangle
test-puzzles: puzzles-host c4bb-images
	@rm -rf .puz_t && mkdir -p .puz_t/d && \
	 for g in $(PUZ_GAMES); do .puzzles/host/bin/$$g --test $(PUZ_STEPS); done > .puz_t/host.txt && \
	 grep -c '^puzzle end' .puz_t/host.txt && \
	 { for g in $(PUZ_GAMES); do echo "$$g --test $(PUZ_STEPS)"; done; echo exit; } \
	   | timeout 3000 node libjs/cli.js -m 64 -d src/c4bb/images/disk src/c4bb/images/c4ix32.c4r 2>/dev/null \
	   | tr -d '\r' | grep '^puzzle' > .puz_t/c4ix.txt; \
	 cmp .puz_t/host.txt .puz_t/c4ix.txt || { echo "test-puzzles: C4IX's differ:"; diff .puz_t/host.txt .puz_t/c4ix.txt | head -20; exit 1; }; \
	 for g in $(PUZ_TEST_BARE); do \
	   ./c4rlink32 $(PUZ_CORE:%=.puzzles/%.c4o) .puzzles/$$g.c4o .puzzles/fe.c4o .puzzles/plat/plat-bare.c4o .libc-bare/libc-*.c4o -o .puz_t/d/$$g.c4r > /dev/null || exit 1; \
	   .puzzles/host/bin/$$g --test $(PUZ_STEPS) > .puz_t/h.txt; \
	   ( cd .puz_t/d && timeout 900 node ../../libjs/cli.js -m 64 -d . $$g.c4r --test $(PUZ_STEPS) 2>/dev/null ) | grep '^puzzle' > .puz_t/b.txt; \
	   cmp .puz_t/h.txt .puz_t/b.txt || { echo "test-puzzles: $$g on the bare machine differs:"; diff .puz_t/h.txt .puz_t/b.txt | head; exit 1; }; \
	   ( cd .puz_t/d && printf '%s --test $(PUZ_STEPS)\nver\n' $$g | timeout 900 node ../../libjs/cli.js -m 64 -d . ../../src/c4bb/images/c4dos32.c4r 2>/dev/null ) | tr -d '\r' > .puz_t/dos.out; \
	   grep '^puzzle' .puz_t/dos.out > .puz_t/b.txt; \
	   cmp .puz_t/h.txt .puz_t/b.txt || { echo "test-puzzles: $$g under C4DOS differs:"; diff .puz_t/h.txt .puz_t/b.txt | head; tail -3 .puz_t/dos.out; exit 1; }; \
	   grep -A2 '^puzzle end' .puz_t/dos.out | grep -q '^C4DOS version' || { echo "test-puzzles: C4DOS did not come back to its prompt"; tail -3 .puz_t/dos.out; exit 1; }; \
	 done; \
	 rm -rf .puz_t && echo "test-puzzles: OK"
PHONY += test-puzzles

GAME_OBJS  += puzzles-objs
GAME_TESTS += test-puzzles
