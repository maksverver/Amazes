#include "MazeIO.h"
#include <assert.h>
#include <string.h>

/* Determines the valid width of the given line, when parsed as consisting
   of horizontal walls. */
static int hor_walls_width(const char *line)
{
    int i;
    if (line[0] != '+')
        return 0;
    for (i = 1; i < 2*WIDTH; i += 2)
    {
        if (strchr(" ?-", line[i]) == NULL || line[i + 1] != '+')
            break;
    }
    return i/2;
}

/* Determines the valid width of the given line, when parsed as consisting
   of vertical walls and squares. */
static int ver_walls_width(const char *line)
{
    int i;
    if (strchr(" ?|", line[0]) == NULL)
        return 0;
    for (i = 1; i < 2*WIDTH; i += 2)
    {
        if (strchr(" ?^>v<", line[i]) == NULL ||
            strchr(" ?|", line[i + 1]) == NULL)
            break;
    }
    return i/2;
}

bool mm_scan(MazeMap *mm, FILE *fp)
{
    char line[512];
    int W, H, n;

    /* Read first line to determine grid width */
    if (fgets(line, sizeof(line), fp) == NULL)
        return false;
    W = hor_walls_width(line);
    if (W < 1) return false;

    /* Start parsing */
    mm_clear(mm);
    for (H = 0; ; ++H)
    {
        /* Parse horizontal walls */
        if (hor_walls_width(line) < W)
            return false;

        for (n = 0; n < W; ++n)
        {
            int val;
            switch (line[2*n + 1])
            {
            case ' ': val = ABSENT;  break;
            case '?': val = UNKNOWN; break;
            case '-': val = PRESENT; break;
            default: assert(0);
            }
            mm->grid[H%HEIGHT][n].wall_n = val;
        }

        /* Read next line (unless we're at the end of input) */
        if (H == HEIGHT || fgets(line, sizeof(line), fp) == NULL ||
            ver_walls_width(line) < W) break;

        /* Parse vertical walls and squares */
        for (n = 0; n <= W; ++n)
        {
            int val;
            switch (line[2*n])
            {
            case ' ': val = ABSENT;  break;
            case '?': val = UNKNOWN; break;
            case '|': val = PRESENT; break;
            default: assert(0);
            }
            mm->grid[H][n%WIDTH].wall_w = val;
        }
        for (n = 0; n < W; ++n)
        {
            int val;
            switch (line[2*n + 1])
            {
            case '^': mm->dir = NORTH; goto set_loc;
            case '>': mm->dir = EAST;  goto set_loc;
            case '<': mm->dir = WEST;  goto set_loc;
            case 'v': mm->dir = SOUTH;
            set_loc:  mm->loc.r = H;
                      mm->loc.c = n;
                      val       = PRESENT;
                      break;
            case ' ': val = PRESENT; break;
            case '?': val = UNKNOWN; break;
            default: assert(0);
            }
            mm->grid[H][n].square = val;
        }

        /* Read next line */
        if (fgets(line, sizeof(line), fp) == NULL)
            return false;
    }
    if (H < 1) return false;

    /* Set border */
    mm->border.top    = 0;
    mm->border.left   = 0;
    mm->border.bottom = H%HEIGHT;
    mm->border.right  = W%WIDTH;

    return true;
}

void mm_print(MazeMap *mm, FILE *fp, bool full)
{
    const int top    = full ? 0 : mm->border.top;
    const int left   = full ? 0 : mm->border.left;
    const int bottom = full ? 0 : mm->border.bottom;
    const int right  = full ? 0 : mm->border.right;
    const int h = (bottom == top) ? HEIGHT : (bottom - top + HEIGHT)%HEIGHT;
    const int w = (right == left) ? WIDTH : (right - left + WIDTH)%WIDTH;

    int r, c, i, j, v;

    for (i = 0; i < h; ++i)
    {
        r = (top + i)%HEIGHT;
        for (j = 0; j < w; ++j)
        {
            c = (left + j)%WIDTH;
            fputc('+', fp);
            v = WALL(mm, r, c, NORTH);
            fputc(v == PRESENT ? '-' : v == ABSENT ? ' ' : '?', fp);
        }
        fputc('+', fp);
        fputc('\n', fp);
        for (j = 0; j < w; ++j)
        {
            c = (left + j)%WIDTH;
            v = WALL(mm, r, c, WEST);
            fputc(v == PRESENT ? '|' : v == ABSENT ? ' ' : '?', fp);
            v = SQUARE(mm, r, c);
            if (r == mm->loc.r && c == mm->loc.c)
            {
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
        }
        c = (left + j)%WIDTH;
        v = WALL(mm, r, c, WEST);
        fputc(v == PRESENT ? '|' : v == ABSENT ? ' ' : '?', fp);
        fputc('\n', fp);
    }

    /* Line 2r */
    r = (top + i)%HEIGHT;
    for (j = 0; j < w; ++j)
    {
        c = (left + j)%WIDTH;
        fputc('+', fp);
        v = WALL(mm, r, c, NORTH);
        fputc(v == PRESENT ? '-' : v == ABSENT ? ' ' : '?', fp);
    }
    fputc('+', fp);
    fputc('\n', fp);
}

bool mm_decode(MazeMap *mm, const char *desc)
{
    char buf[512], *p;
    int H, W, r, c, val, len, pass;

    /* decode base-64 chars (into integers in range [0,64)) */
    strncpy(buf, desc, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    for (p = buf; *p; ++p)
    {
        if (*p >= 'A' && *p <= 'Z') *p = *p - 'A' +  0;
        else
        if (*p >= 'a' && *p <= 'z') *p = *p - 'a' + 26;
        else
        if (*p >= '0' && *p <= '9') *p = *p - '0' + 52;
        else
        if (*p == '-')              *p =            62;
        else
        if (*p == '_')              *p =            63;
        else
            return false;  /* invalid char */
    }

    /* Parse first five fields */
    mm_clear(mm);
    H = buf[0];
    W = buf[1];
    if (H > HEIGHT) return false;
    if (W > WIDTH)  return false;
    mm->border.top    = 0;
    mm->border.left   = 0;
    mm->border.bottom = H%HEIGHT;
    mm->border.right  = W%WIDTH;
    if (buf[2] && buf[3]) {
        mm->loc.r = buf[2] - 1;
        mm->loc.c = buf[3] - 1;
    }
    if (mm->loc.r >= H) return false;
    if (mm->loc.c >= W) return false;
    mm->dir = (Dir)(buf[4]&3);
    p = &buf[5];

    /* Parse squares */
    val = len = 0;
    for (r = 0; r < H; ++r)
    {
        for (c = 0; c < W; ++c)
        {
            if (len == 0)
            {
                val = *p++;
                len = 6;
            }
            mm->grid[r][c].square = (val%2) ? PRESENT : UNKNOWN;
            val /= 2;
            --len;
        }
    }

    /* Parse walls */
    val = len = 0;
    for (pass = 0; pass < 2; ++pass)
    {
        for (r = 0; r < H + (pass == 0 ? 1 : 0); ++r)
        {
            for (c = 0; c < W + (pass == 0 ? 0 : 1); ++c)
            {
                if (len == 0)
                {
                    val = p[0] + 64*p[1] + 64*64*p[2] + 64*64*64*p[3];
                    p += 4;
                    len = 15;
                }

                if (pass == 0)
                    mm->grid[r%HEIGHT][c].wall_n = val%3 - 1;
                else /* pass == 1 */
                    mm->grid[r][c%WIDTH].wall_w = val%3 - 1;

                val /= 3;
                --len;
            }
        }
    }

    return true;
}

const char *mm_encode(MazeMap *mm, bool full)
{
    static char buf[512];

    const int top    = full ? 0 : mm->border.top;
    const int left   = full ? 0 : mm->border.left;
    const int bottom = full ? 0 : mm->border.bottom;
    const int right  = full ? 0 : mm->border.right;

    const int h = (bottom == top) ? HEIGHT : (bottom - top + HEIGHT)%HEIGHT;
    const int w = (right == left) ? WIDTH : (right - left + WIDTH)%WIDTH;

    char *p = buf;
    int val, len, r, c, i, j, pass, n;

    static const char *base64_digits = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdef"
                                       "ghijklmnopqrstuvwxyz0123456789-_";

    static const int pow3[15] = { 1, 3, 9, 27, 81, 243, 729, 2187, 6561, 19683,
                                  59049, 177147, 531441, 1594323, 4782969 };


    /* Encode first five fields: */
    *p++ = base64_digits[h];
    *p++ = base64_digits[w];
    *p++ = base64_digits[1 + (mm->loc.r - top + HEIGHT)%HEIGHT];
    *p++ = base64_digits[1 + (mm->loc.c - left + WIDTH)%WIDTH];
    *p++ = base64_digits[(int)mm->dir];

    /* Encode squares */
    val = len = 0;
    for (i = 0; i < h; ++i)
    {
        for (j = 0; j < w; ++j)
        {
            r = (top  + i)%HEIGHT;
            c = (left + j)%WIDTH;
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
                r = (top  + i)%HEIGHT;
                c = (left + j)%WIDTH;
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
