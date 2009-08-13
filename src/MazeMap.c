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
        return mm->grid[r][(dir == EAST) ? (c + 1)%25 : c].wall_e;
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
        mm->grid[r][(dir == EAST) ? (c + 1)%25 : c].wall_e = val;
    }
}

void mm_clear(MazeMap *mm)
{
    memset(mm, 0, sizeof(MazeMap));
    mm->dir   = NORTH;
    mm->loc.r = 0;
    mm->loc.c = 0;
    SET_SQUARE(mm, 0, 0, PRESENT);
}

void mm_add_line(MazeMap *mm, const char *line, RelDir rel_dir)
{
    Dir dir   = TURN(mm->dir, rel_dir),
        left  = TURN(dir, LEFT),
        right = TURN(dir, RIGHT);

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
            SET_WALL(mm, r, c, FRONT, PRESENT);
            assert(*line == '\0');
            return;
        default:  /* invalid char */
            assert(0);
        }

        r = RDR(r, dir);
        c = CDC(c, dir);

        SET_SQUARE(mm, r, c, PRESENT);
        SET_WALL(mm, r, c, BACK, ABSENT);

        if (open_left)
        {
            SET_WALL(mm, r, c, LEFT, PRESENT);
            SET_SQUARE(mm, RDR(r, left), CDC(c, left), PRESENT);
        }

        if (open_right)
        {
            SET_WALL(mm, r, c, RIGHT, PRESENT);
            SET_SQUARE(mm, RDR(r, right), CDC(c, right), PRESENT);
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
        mm->loc.r = CDC(mm->loc.c, mm->dir);
    }
}
