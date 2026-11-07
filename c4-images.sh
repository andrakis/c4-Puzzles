# c4-images.sh c4ix|bare -- the puzzles' part of c4's build-images.sh, which
# sources it from c4's root (games/README.md in c4). One image a game: the
# core, its unit and the front end, of which c4rlink keeps what it reaches.
# They have no data.
PUZ=games/puzzles
PUZ_OBJS=$(sed 's,^,.puzzles/,; s,$,.c4o,' $PUZ/c4/core.txt)
case "$1" in
c4ix)
    if ls .puzzles/*.c4o > /dev/null 2>&1 && [ -f $PB/lib/libc.c4s ]; then
        say "linking the puzzles for C4IX"
        for g in $(cat $PUZ/c4/games.txt); do
            ./c4rlink32 $PUZ_OBJS .puzzles/$g.c4o .puzzles/fe.c4o .puzzles/plat/plat-c4ix.c4o \
                -l $PB/lib/libc.c4s -o $DISK/c4ix-puz-$g.c4r > /dev/null
        done
        ./c4rlink32 .puzzles/plat/puzzles-list.c4o -l $PB/lib/libc.c4s -o $DISK/c4ix-puzzles.c4r > /dev/null
    fi ;;
bare)
    if ls .puzzles/*.c4o > /dev/null 2>&1; then
        # the same selection as C4IX's would fill the games disk: C4DOS and the
        # bare machine take a few (make test-puzzles links the rest it wants)
        for g in net fifteen mines; do
            item $g
            ./c4rlink32 $PUZ_OBJS .puzzles/$g.c4o .puzzles/fe.c4o .puzzles/plat/plat-bare.c4o \
                .libc-bare/libc-*.c4o -o $GAMESDISK/$g.c4r > /dev/null
        done
        GAMES_ON_DISK="$GAMES_ON_DISK NET FIFTEEN MINES"
    fi ;;
esac
