#include "MazeMap.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>

int dir_dr[4] = { -1,  0, +1,  0 };
int dir_dc[4] = {  0, +1,  0, -1 };

int mm_get_wall(const MazeMap *mm, int r, int c, Dir dir)
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

void mm_clear_squares(MazeMap *mm)
{
    int r, c;
    for (r = 0; r < HEIGHT; ++r)
    {
        for (c = 0; c < WIDTH; ++c)
        {
            mm->grid[r][c].square = UNKNOWN;
        }
    }
}

void mm_clear(MazeMap *mm)
{
    memset(mm, 0, sizeof(MazeMap));
}

void mm_initialize(MazeMap *mm, int r, int c, Dir dir)
{
    mm_clear(mm);
    mm->loc.r = r;
    mm->loc.c = c;
    mm->dir   = dir;
    mm->border.top    = r;
    mm->border.left   = c;
    mm->border.bottom = (r + 1)%WIDTH;
    mm->border.right  = (c + 1)%HEIGHT;
    SET_SQUARE(mm, r, c, PRESENT);
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

void mm_move(MazeMap *mm, char move)
{
    RelDir rel_dir;
    switch (move)
    {
    case 'F': rel_dir = FRONT; break;
    case 'T': rel_dir = BACK;  break;
    case 'L': rel_dir = LEFT;  break;
    case 'R': rel_dir = RIGHT; break;
    default: assert(0);   /* invalid char */
    }
    mm->dir = TURN(mm->dir, rel_dir);
    push_border(mm, mm->loc.r, mm->loc.c, mm->dir);
    mm->loc.r = RDR(mm->loc.r, mm->dir);
    mm->loc.c = CDC(mm->loc.c, mm->dir);
    SET_SQUARE(mm, mm->loc.r, mm->loc.c, PRESENT);
}

void mm_turn(MazeMap *mm, const char *turn)
{
    while (*turn) mm_move(mm, *turn++);
}

static bool mark_dead_end(MazeMap *mm, bool dead_end[HEIGHT][WIDTH],
                          int r, int c)
{
    int num_walls = 0, num_dead_adjacent = 0, dir;
    bool changed = false;

    if (dead_end[r][c]) return false;

    for (dir = 0; dir < 4; ++dir)
    {
        int w = WALL(mm, r, c, dir);
        if (w == PRESENT)
            ++num_walls;
        else
        if (w == ABSENT && dead_end[RDR(r, dir)][CDC(c, dir)])
            ++num_dead_adjacent;
    }
    assert(num_walls + num_dead_adjacent < 4);

    if (num_walls + num_dead_adjacent == 3)
    {
        dead_end[r][c] = true;
        if (SQUARE(mm, r, c) == UNKNOWN)
        {
            SET_SQUARE(mm, r, c, PRESENT);
            changed = true;
        }
        for (dir = 0; dir < 4; ++dir)
        {
            if (WALL(mm, r, c, dir) == UNKNOWN)
            {
                SET_WALL(mm, r, c, dir, ABSENT);
                changed = true;
            }
        }
        for (dir = 0; dir < 4; ++dir)
        {
            if (WALL(mm, r, c, dir) == ABSENT)
            {
                if (mark_dead_end(mm, dead_end, RDR(r, dir), CDC(c, dir)))
                    changed = true;
            }
        }
    }

    return changed;
}

void mm_infer(MazeMap *mm)
{
    bool changed;
    int  r, c;
    bool dead_end[HEIGHT][WIDTH];

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
        memset(dead_end, 0, sizeof(dead_end));
        for (r = 0; r < HEIGHT; ++r)
        {
            for (c = 0; c < WIDTH; ++c)
            {
                if (mark_dead_end(mm, dead_end, r, c))
                    changed = true;
            }
        }

        /* Discovery of walls */

        /* If for a corner in the maze you have discovered that three edges are
           openings, the fourth edge has to be a wall and is discovered. */
        for (r = 0; r < HEIGHT; ++r)
        {
            for (c = 0; c < WIDTH; ++c)
            {
                const int r1 = (r + 1)%HEIGHT;
                const int c1 = (c + 1)%WIDTH;
                int n, e, s, w, na;
                n = mm->grid[r ][c1].wall_w;
                e = mm->grid[r1][c1].wall_n;
                s = mm->grid[r1][c1].wall_w;
                w = mm->grid[r1][c ].wall_n;
                na = 0;
                if (n == ABSENT) ++na;
                if (e == ABSENT) ++na;
                if (s == ABSENT) ++na;
                if (w == ABSENT) ++na;
                assert(na <= 3);
                if (na == 3)
                {
                    if (n == UNKNOWN)
                    {
                        mm->grid[r ][c1].wall_w = PRESENT;
                        changed = true;
                    }
                    else
                    if (e == UNKNOWN)
                    {
                        mm->grid[r1][c1].wall_n = PRESENT;
                        changed = true;
                    }
                    else
                    if (s == UNKNOWN)
                    {
                        mm->grid[r1][c1].wall_w = PRESENT;
                        changed = true;
                    }
                    else
                    if (w == UNKNOWN)
                    {
                        mm->grid[r1][c ].wall_n = PRESENT;
                        changed = true;
                    }
                }
            }
        }

        /* If you have discovered, or know of open connections to, at least one
           square in all 25 columns of the maze, the vertical walls on the
           outside (the outer edges of the maze) are discovered. */
        if (mm->border.left == mm->border.right)
        {
            for (r = 0; r < HEIGHT; ++r)
            {
                if (mm->grid[r][mm->border.left].wall_w != PRESENT)
                {
                    assert(mm->grid[r][mm->border.left].wall_w == UNKNOWN);
                    mm->grid[r][mm->border.left].wall_w = PRESENT;
                    changed = true;
                }
            }
        }

        /* If you have discovered, or know of open connections to, at least one
           square in all 25 rows of the maze, the horizontal walls on the
           outside (the outer edges of the maze) are discovered. */
        if (mm->border.top == mm->border.bottom)
        {
            for (c = 0; c < WIDTH; ++c)
            {
                if (mm->grid[mm->border.top][c].wall_n != PRESENT)
                {
                    assert(mm->grid[mm->border.top][c].wall_n == UNKNOWN);
                    mm->grid[mm->border.top][c].wall_n = PRESENT;
                    changed = true;
                }
            }
        }

        /* If the 25 walls of one outer edge of the maze have been discovered,
           the 25 walls on the opposing side are also determined as discovered.
        */
        /* (No need to code; this works automatically in the current state
            representation, and the benefit is unclear anyway.) */
    } while (changed);
}

int mm_count_squares(const MazeMap *mm)
{
    int r, c, res = 0;
    for (r = 0; r < HEIGHT; ++r)
    {
        for (c = 0; c < WIDTH; ++c)
        {
            if (mm->grid[r][c].square == PRESENT)
                ++res;
        }
    }
    return res;
}

int mm_width(const MazeMap *mm)
{
    int w = (mm->border.right - mm->border.left + WIDTH)%WIDTH;
    return w ? w : WIDTH;
}

int mm_height(const MazeMap *mm)
{
    int h = (mm->border.bottom - mm->border.top + WIDTH)%WIDTH;
    return h ? h : WIDTH;
}
