// puzzles-list.c -- `puzzles` on C4IX: what is here. The names and the lines are
// upstream's, from its CMakeLists.txt. Each game is a program of its own.
#include <stdio.h>

static const char *const games[][2] = {
    { "blackbox", "Ball-finding puzzle" },
    { "bridges", "Bridge-placing puzzle" },
    { "cube", "Rolling cube puzzle" },
    { "dominosa", "Domino tiling puzzle" },
    { "fifteen", "Sliding block puzzle" },
    { "filling", "Polyomino puzzle" },
    { "flip", "Tile inversion puzzle" },
    { "flood", "Flood-filling puzzle" },
    { "galaxies", "Symmetric polyomino puzzle" },
    { "guess", "Combination-guessing puzzle" },
    { "inertia", "Gem-collecting puzzle" },
    { "keen", "Arithmetic Latin square puzzle" },
    { "lightup", "Light-bulb placing puzzle" },
    { "loopy", "Loop-drawing puzzle" },
    { "magnets", "Magnet-placing puzzle" },
    { "map", "Map-colouring puzzle" },
    { "mines", "Mine-finding puzzle" },
    { "mosaic", "Grid-filling puzzle" },
    { "net", "Network jigsaw puzzle" },
    { "netslide", "Toroidal sliding network puzzle" },
    { "palisade", "Grid-division puzzle" },
    { "pattern", "Pattern puzzle" },
    { "pearl", "Loop-drawing puzzle" },
    { "pegs", "Peg solitaire puzzle" },
    { "range", "Visible-distance puzzle" },
    { "rect", "Rectangles puzzle" },
    { "samegame", "Block-clearing puzzle" },
    { "signpost", "Square-connecting puzzle" },
    { "singles", "Number-removing puzzle" },
    { "sixteen", "Toroidal sliding block puzzle" },
    { "slant", "Maze-drawing puzzle" },
    { "solo", "Number placement puzzle" },
    { "tents", "Tent-placing puzzle" },
    { "towers", "Tower-placing Latin square puzzle" },
    { "tracks", "Path-finding railway track puzzle" },
    { "twiddle", "Rotational sliding block puzzle" },
    { "undead", "Monster-placing puzzle" },
    { "unequal", "Latin square puzzle" },
    { "unruly", "Black and white grid puzzle" },
    { "untangle", "Planar graph layout puzzle" },
};

int main(void)
{
    int i;
    printf("Simon Tatham's Portable Puzzle Collection. Type a name to play:\n\n");
    for (i = 0; i < (int)(sizeof(games) / sizeof(games[0])); i++)
        printf("  %-10s %s\n", games[i][0], games[i][1]);
    printf("\n`net 7x7` takes a size or a game ID. `net --window` plays in this Terminal.\n");
    return 0;
}
