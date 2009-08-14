#include "MazeIO.h"
#include <assert.h>

void mm_print(MazeMap *mm, FILE *fp)
{
    int r, c, v;
    r = mm->border.top;
    do {
        /* Line 2r */
        c = mm->border.left;
        do {
            fputc('+', fp);
            v = WALL(mm, r, c, NORTH);
            fputc(v == PRESENT ? '-' : v == ABSENT ? ' ' : '?', fp);
        } while ((c = (c + 1)%WIDTH) != mm->border.right);
        fputc('+', fp);
        fputc('\n', fp);

        /* Line 2r + 1 */
        c = mm->border.left;
        do {
            v = WALL(mm, r, c, WEST);
            fputc(v == PRESENT ? '|' : v == ABSENT ? ' ' : '?', fp);
            v = SQUARE(mm, r, c);
            if (r == mm->loc.r && c == mm->loc.c)
            {
                assert(v == PRESENT);
                switch (mm->dir)
                {
                case NORTH: fputc('^', fp); break;
                case EAST:  fputc('>', fp); break;
                case SOUTH: fputc('v', fp); break;
                case WEST:  fputc('<', fp); break;
                default: assert(0);
                }
            }
            else
            {
                assert(v != ABSENT);
                fputc(v == PRESENT ? ' ' : '?', fp);
            }
        } while ((c = (c + 1)%WIDTH) != mm->border.right);
        v = WALL(mm, r, c, WEST);
        fputc(v == PRESENT ? '|' : v == ABSENT ? ' ' : '?', fp);
        fputc('\n', fp);
    } while ((r = (r + 1)%HEIGHT) != mm->border.bottom);

    /* Line 2r */
    c = mm->border.left;
    do {
        fputc('+', fp);
        v = WALL(mm, r, c, NORTH);
        fputc(v == PRESENT ? '-' : v == ABSENT ? ' ' : '?', fp);
    } while ((c = (c + 1)%WIDTH) != mm->border.right);
    fputc('+', fp);
    fputc('\n', fp);
}

const char *mm_encode(MazeMap *mm)
{
    static char buf[512];
    char *p = buf;
    int h, w, val, len, r, c, i, j, pass, n;

    static const char *base64_digits = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdef"
                                       "ghijklmnopqrstuvwxyz0123456789-_";

    static const int pow3[15] = { 1, 3, 9, 27, 81, 243, 729, 2187, 6561, 19683,
                                  59049, 177147, 531441, 1594323, 4782969 };

    h = (mm->border.bottom - mm->border.top + HEIGHT)%HEIGHT;
    if (h == 0) h = HEIGHT;
    w = (mm->border.right - mm->border.left + WIDTH)%WIDTH;
    if (w == 0) w = WIDTH;

    /* Encode first five fields: */
    *p++ = base64_digits[h];
    *p++ = base64_digits[w];
    *p++ = base64_digits[(mm->loc.r - mm->border.top + HEIGHT)%HEIGHT];
    *p++ = base64_digits[(mm->loc.c - mm->border.left + WIDTH)%WIDTH];
    *p++ = base64_digits[(int)mm->dir];

    /* Encode squares */
    val = len = 0;
    for (i = 0; i < h; ++i)
    {
        for (j = 0; j < w; ++j)
        {
            r = (mm->border.top  + i)%HEIGHT;
            c = (mm->border.left + j)%WIDTH;
            val |= SQUARE(mm, r, c) << len++;
            if (len == 6)
            {
                *p++ = base64_digits[val];
                val = len = 0;
            }
        }
    }

    if (len > 0)
        *p++ = base64_digits[val];

    /* Encode walls */
    val = len = 0;
    for (pass = 0; pass < 2; ++pass)
    {
        for (i = 0; i < h + (pass ? 0 : 1); ++i)
        {
            for (j = 0; j < w + (pass ? 1 : 0); ++j)
            {
                r = (mm->border.top  + i)%HEIGHT;
                c = (mm->border.left + j)%WIDTH;
                if (pass == 0)  /* horizontal walls */
                    val += pow3[len++]*(mm->grid[r][c].wall_n + 1);
                else  /* pass == 1: vertical walls */
                    val += pow3[len++]*(mm->grid[r][c].wall_w + 1);
                if (len == 15)
                {
                    for (n = 0; n < 4; ++n)
                    {
                        *p++ = base64_digits[val%64];
                        val /= 64;
                    }
                    assert(val == 0);
                    len = 0;
                }
            }
        }
    }
    while (val > 0)
    {
        *p++ = base64_digits[val%64];
        val /= 64;
    }

    *p = '\0';
    return buf;
}
