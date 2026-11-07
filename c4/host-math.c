// host-math.c -- for the gcc build, C4IX libc's math.c in place of glibc's, as
// plat-host.c's qsort is. Both are within an ulp of the truth, but not the
// same one, and Untangle places a point with (long)(c - r * cos(a) + 0.5),
// which lands exactly on a whole number. So the only difference left between
// the two builds make test-puzzles compares is the compiler. It makes a test
// of math.c too: the arithmetic here is SSE's, and on the machine, soft float's.
#define errno c4_errno
int c4_errno;
static unsigned long long __c4d_sqrt(unsigned long long b)
{
    double d = *(double *)&b;
    __asm__("sqrtsd %0, %0" : "+x" (d));
    return *(unsigned long long *)&d;
}
#include "src/c4ix/libc/math.c"
