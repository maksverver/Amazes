#include "Analysis.h"

#define MAX_TURNS 150

const char *pick_move(MazeMap *mm, int distsq)
{
    int r, c, dir;
    int dist[HEIGHT][WIDTH];
    int best_r = 0, best_c = 0, best_v = 0, best_dist = 0;

    (void)distsq; /* unused */

    find_distance(mm, dist, mm->loc.r, mm->loc.c);

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
                    best_r = r;
                    best_c = c;
                }
            }
        }
    }

    if (best_v == 0) return "T";

    return construct_turn(mm, dist, mm->loc.r, mm->loc.c, mm->dir,
                          best_r, best_c);
}
