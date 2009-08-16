#include "MazeMap.h"
#include "MazeIO.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_TURNS 150

static MazeMap mm;

static void bfs(int dist[HEIGHT][WIDTH], int r, int c)
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
            if (WALL(&mm, r, c, dir) == ABSENT)
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

static const char *construct_turn( int dist[HEIGHT][WIDTH],
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
            if (WALL(&mm, r2, c2, dir) == ABSENT)
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

static const char *pick_move()
{
    int r, c, dir;
    int dist[HEIGHT][WIDTH];
    int best_r = 0, best_c = 0, best_v = 0, best_dist = 0;

    bfs(dist, mm.loc.r, mm.loc.c);

    for (r = 0; r < HEIGHT; ++r)
    {
        for (c = 0; c < WIDTH; ++c)
        {
            if (dist[r][c] != -1)
            {
                int v = 0;
                for (dir = 0; dir < 4; ++dir)
                {
                    if (SQUARE(&mm, RDR(r, dir), CDC(c, dir)) == UNKNOWN &&
                        WALL(&mm, r, c, dir) != PRESENT) ++v;
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

    return construct_turn(dist, mm.loc.r, mm.loc.c, mm.dir, best_r, best_c);
}

static char *get_line(bool remove)
{
    static char buf[1024];
    static char *cur_line = NULL;
    char *res;

    if (cur_line == NULL)
    {
        char *eol;
        if (fgets(buf, sizeof(buf), stdin) == NULL ||
            (eol = strchr(buf, '\n')) == NULL)
        {
            fprintf(stderr, "Could not read the next line! Exiting.\n");
            exit(EXIT_FAILURE);
        }

        /* Remove trailing whitespace */
        while (eol > buf && isspace(*(eol - 1))) --eol;
        *eol = '\0';

        if (strcmp(buf, "Quit") == 0)
        {
            fprintf(stderr, "Received Quit.\n");
            exit(EXIT_SUCCESS);
        }

        cur_line = buf;
    }

    res = cur_line;
    if (remove) cur_line = NULL;
    return res;
}

static void read_input()
{
    int sq_dist;
    mm_look(&mm, get_line(true), FRONT);
    mm_look(&mm, get_line(true), RIGHT);
    mm_look(&mm, get_line(true), BACK);
    mm_look(&mm, get_line(true), LEFT);
    sscanf(get_line(true), "%d", &sq_dist);
}

static void write_output(const char *move)
{
    mm_turn(&mm, move);
    fprintf(stdout, "%s\n", move);
    fflush(stdout);
}

int main()
{
    int turn;

    mm_initialize(&mm, 0, 0, NORTH);

    if (strcmp(get_line(false), "Start") == 0)
    {
        /* I start -- not sure what good that does me though. */
        get_line(true);
    }

    for (turn = 0; ; ++turn)
    {
        read_input();
        /*
        mm_print(&mm, stderr);
        fprintf(stderr, "%s\n", mm_encode(&mm));
        fflush(stderr);
        */

        if (turn == MAX_TURNS - 1)
        {
            write_output("T");
            break;
        }

        write_output(pick_move());
    }
    return 0;
}
