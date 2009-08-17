#include "Analysis.h"
#include <assert.h>

#define MAX_TURNS 150

static int dist[HEIGHT][WIDTH];

static Point explore(MazeMap *mm)
{
    Point res;
    int r, c, dir;
    int best_v = 0, best_dist = 0;
    for (r = 0; r < HEIGHT; ++r)
    {
        for (c = 0; c < WIDTH; ++c)
        {
            if (dist[r][c] != -1)
            {
                int v = 0;
                for (dir = 0; dir < 4; ++dir)
                {
                    if (SQUARE(mm, RDR(r, dir), CDC(c, dir)) == UNKNOWN &&
                        WALL(mm, r, c, dir) != PRESENT) ++v;
                }
                if (v > best_v || (v == best_v && dist[r][c] < best_dist))
                {
                    best_v = v;
                    best_dist = dist[r][c];
                    res.r = r;
                    res.c = c;
                }
            }
        }
    }
    assert(best_v > 0);
    return res;
}

static Point squash(MazeMap *mm, int distsq)
{
    Point res;
    int r, c, best_dist = -1;

    if (distsq <= 0) return mm->loc;

    for (r = 0; r < HEIGHT; ++r)
    {
        for (c = 0; c < WIDTH; ++c)
        {
            const int dr = (r - mm->border.top + HEIGHT)%HEIGHT -
                           (mm->loc.r - mm->border.top + HEIGHT)%HEIGHT,
                      dc = (c - mm->border.left + WIDTH)%WIDTH -
                           (mm->loc.c - mm->border.left + WIDTH)%WIDTH;

            if (dr*dr + dc*dc == distsq)
            {
                int r = (mm->loc.r + dr + HEIGHT)%HEIGHT,
                    c = (mm->loc.c + dc + WIDTH)%WIDTH;
                if (best_dist == -1 || dist[r][c] < best_dist)
                {
                    best_dist = dist[r][c];
                    res.r = r;
                    res.c = c;
                }
            }
        }
    }
    assert(best_dist != -1);
    return res;
}

const char *pick_move(MazeMap *mm, int distsq)
{
    Point dst;

    find_distance(mm, dist, mm->loc.r, mm->loc.c);

    if (mm_count_squares(mm) < WIDTH*HEIGHT)
        dst = explore(mm);
    else /* mm_count_squares(mm) == WIDTH*HEIGHT */
        dst = squash(mm, distsq);

    if (dst.r == mm->loc.r && dst.c == mm->loc.c)
        return "T";

    return construct_turn(mm, dist, mm->loc.r, mm->loc.c, mm->dir, dst.r, dst.c);
}
