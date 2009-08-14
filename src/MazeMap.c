#include "MazeMap.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>

int dir_dr[4] = { -1,  0, +1,  0 };
int dir_dc[4] = {  0, +1,  0, -1 };

int mm_get_wall(MazeMap *mm, int r, int c, Dir dir)
{
    if (((int)dir&1) == 0)  /* north/south */
    {
        return mm->grid[(dir == NORTH) ? r : (r + 1)%25][c].wall_n;
    }
    else  /* east/west */
    {
        return mm->grid[r][(dir == WEST) ? c : (c + 1)%25].wall_w;
    }
}

void mm_set_wall(MazeMap *mm, int r, int c, Dir dir, int val)
{
    if (((int)dir&1) == 0)  /* north/south */
    {
        mm->grid[(dir == NORTH) ? r : (r + 1)%25][c].wall_n = val;
    }
    else  /* east/west */
    {
        mm->grid[r][(dir == WEST) ? c : (c + 1)%25].wall_w = val;
    }
}

void mm_clear(MazeMap *mm)
{
    memset(mm->grid, 0, sizeof(mm->grid));
    SET_SQUARE(mm, 0, 0, PRESENT);
    mm->loc.r = 0;
    mm->loc.c = 0;
    mm->dir   = NORTH;
    mm->border.left   = 0;
    mm->border.top    = 0;
    mm->border.right  = 1;
    mm->border.bottom = 1;
    mm->bound_ns = false;
    mm->bound_ew = false;
}

static void push_border(MazeMap *mm, int r, int c, Dir dir)
{
    switch (dir)
    {
    case NORTH:
        if (r == mm->border.top)
            mm->border.top = (mm->border.top + HEIGHT - 1)%HEIGHT;
        break;
    case EAST:
        if ((c + 1)%WIDTH == mm->border.right)
            mm->border.right = (mm->border.right + 1)%WIDTH;
        break;
    case SOUTH:
        if ((r + 1)%HEIGHT == mm->border.bottom)
            mm->border.bottom = (mm->border.bottom + 1)%HEIGHT;
        break;
    case WEST:
        if (c == mm->border.left)
            mm->border.left = (mm->border.left + WIDTH - 1)%WIDTH;
        break;
    }
}

void mm_look(MazeMap *mm, const char *line, RelDir rel_dir)
{
    Dir front = TURN(mm->dir, rel_dir),
        left  = TURN(front, LEFT),
        right = TURN(front, RIGHT),
        back  = TURN(front, BACK);

    int r = mm->loc.r,
        c = mm->loc.c;

    while (*line)
    {
        bool open_left = false, open_right = false;
        switch (*line++)
        {
        case 'B':  /* opening on both sides */
            open_left = open_right = true;
            break;
        case 'L':  /* opening on left side */
            open_left = true;
            break;
        case 'R':   /* opening on right side */
            open_right = true;
            break;
        case 'N':   /* no opening on either side */
            break;
        case 'W':  /* wall immediately ahead: */
            SET_WALL(mm, r, c, front, PRESENT);
            assert(*line == '\0');
            return;
        default:  /* invalid char */
            assert(0);
        }

        push_border(mm, r, c, front);
        r = RDR(r, front);
        c = CDC(c, front);

        SET_SQUARE(mm, r, c, PRESENT);
        SET_WALL(mm, r, c, back, ABSENT);
        SET_WALL(mm, r, c, left, open_left ? ABSENT : PRESENT);
        SET_WALL(mm, r, c, right, open_right ? ABSENT : PRESENT);

        if (open_left)
        {
            SET_SQUARE(mm, RDR(r, left), CDC(c, left), PRESENT);
            push_border(mm, r, c, left);
        }

        if (open_right)
        {
            SET_SQUARE(mm, RDR(r, right), CDC(c, right), PRESENT);
            push_border(mm, r, c, right);
        }
    }

    assert(0); /* shouldn't get here: every line must end with W */
    return;
}

void mm_move(MazeMap *mm, const char *move)
{
    while (*move)
    {
        assert(SQUARE(mm, mm->loc.r, mm->loc.c) == PRESENT);  /* debug */
        RelDir rel_dir;
        switch (*move++)
        {
        case 'F': rel_dir = FRONT; break;
        case 'T': rel_dir = BACK;  break;
        case 'L': rel_dir = LEFT;  break;
        case 'R': rel_dir = RIGHT; break;
        default: assert(0);   /* invalid char */
        }
        mm->dir = TURN(mm->dir, rel_dir);
        mm->loc.r = RDR(mm->loc.r, mm->dir);
        mm->loc.c = CDC(mm->loc.c, mm->dir);
    }
}

void mm_infer(MazeMap *mm)
{
    bool changed;
    do {
        changed = false;

        /* A square that you have not yet discovered, but for which you have
           discovered three of its walls, is also discovered. This square is
           called a 'dead-end'-square. The opening from this square to the
           next square is also discovered.

           A square that has one discovered opening to a 'dead-end'-square and
           two discovered walls is also discovered. This square is also called
           a 'dead-end'-square. Now both openings are discovered.

           A square that has two discovered openings to 'dead-end'-squares and
           has one discovered wall is also discovered. This square is also
           called a 'dead-end'-square. The three openings from this square are
           also discovered.

           A square that has three discovered openings to 'dead-end'-squares is
           also discovered. This square is also called a 'dead-end'-square. All
           four openings for this square are discovered.
        */
        /* TODO */

        /* Discovery of walls */

        /* If for a corner in the maze you have discovered that three edges are
           openings, the fourth edge has to be a wall and is discovered. */
        {
            int r, c;
            for (r = 0; r < HEIGHT; ++r)
            {
                for (c = 0; c < WIDTH; ++c)
                {
                    int n, e, s, w, na, nu;
                    n = mm->grid[r    ][c + 1].wall_w;
                    e = mm->grid[r + 1][c + 1].wall_n;
                    s = mm->grid[r + 1][c + 1].wall_w;
                    w = mm->grid[r + 1][c    ].wall_n;
                    na = 0;
                    if (n == ABSENT) ++na;
                    if (e == ABSENT) ++na;
                    if (s == ABSENT) ++na;
                    if (w == ABSENT) ++na;
                    nu = 0;
                    assert(na <= 3);
                    if (na == 3)
                    {
                        if (n == UNKNOWN)
                        {
                            mm->grid[r    ][c + 1].wall_w = PRESENT;
                            changed = true;
                        }
                        else
                        if (e == UNKNOWN)
                        {
                            mm->grid[r + 1][c + 1].wall_n = PRESENT;
                            changed = true;
                        }
                        else
                        if (s == UNKNOWN)
                        {
                            mm->grid[r + 1][c + 1].wall_w = PRESENT;
                            changed = true;
                        }
                        else
                        if (w == UNKNOWN)
                        {
                            mm->grid[r + 1][c    ].wall_n = PRESENT;
                            changed = true;
                        }
                    }
                }
            }
            printf("%d\n", changed);
        }
        /* TODO */

        /* If you have discovered, or know of open connections to, at least one
           square in all 25 columns of the maze, the vertical walls on the
           outside (the outer edges of the maze) are discovered. */
        /* TODO */

        /* If you have discovered, or know of open connections to, at least one
           square in all 25 rows of the maze, the horizontal walls on the
           outside (the outer edges of the maze) are discovered. */
        /* TODO */

        /* If the 25 walls of one outer edge of the maze have been discovered,
           the 25 walls on the opposing side are also determined as discovered.
        */
        /* TODO */
    } while (changed);
}

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
