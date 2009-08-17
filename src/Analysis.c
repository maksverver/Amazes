#include "Analysis.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void find_distance(MazeMap *mm, int dist[HEIGHT][WIDTH], int r, int c)
{
    Point queue[HEIGHT*WIDTH];
    int dir, pos = 0, end = 0;
    memset(dist, -1, sizeof(int)*HEIGHT*WIDTH);
    dist[r][c] = 0;
    for (;;)
    {
        assert(dist[r][c] >= 0);
        for (dir = 0; dir < 4; ++dir)
        {
            if (WALL(mm, r, c, dir) == ABSENT)
            {
                int nr = RDR(r, dir), nc = CDC(c, dir);
                if (dist[nr][nc] == -1)
                {
                    dist[nr][nc] = dist[r][c] + 1;
                    queue[end].r = nr;
                    queue[end].c = nc;
                    ++end;
                }
            }
        }

        if (pos == end) break;
        r = queue[pos].r;
        c = queue[pos].c;
        ++pos;
    }
}

const char *construct_turn(MazeMap *mm, int dist[HEIGHT][WIDTH],
                           int r1, int c1, int dir1, int r2, int c2)
{
    static char path_buf[HEIGHT*WIDTH];

    Point loc[HEIGHT*WIDTH];
    int pos, len, dir;

    /* Find path (r1,c1)==pos[0], pos[1], pos[2], .., pos[len]==(r2,c2) */
    loc[0].r = r1, loc[0].c = c1;
    len = dist[r2][c2];
    for (pos = len; pos > 0; --pos)
    {
        loc[pos].r = r2, loc[pos].c = c2;
        for (dir = 0; dir < 4; ++dir)
        {
            if (WALL(mm, r2, c2, dir) == ABSENT)
            {
                int nr = RDR(r2, dir), nc = CDC(c2, dir);
                if (dist[nr][nc] == pos - 1)
                {
                    r2 = nr, c2 = nc;
                    break;
                }
            }
        }
        assert(dir < 4);
    }
    assert(r1 == r2 && c1 == c2);

    /* Build moves: */
    dir = dir1;
    for (pos = 0; pos < len; ++pos)
    {
        int rel_dir;
        for (rel_dir = 0; rel_dir < 4; ++rel_dir)
        {
            int ndir = TURN(dir, rel_dir);
            int nr = RDR(loc[pos].r, ndir), nc = CDC(loc[pos].c, ndir);
            if (nr == loc[pos + 1].r && nc == loc[pos + 1].c)
            {
                dir = ndir;
                break;
            }
        }
        assert(rel_dir < 4);
        path_buf[pos] = "FRTL"[rel_dir];
    }
    path_buf[pos] = '\0';
    return path_buf;
}
