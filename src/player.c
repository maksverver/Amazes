#include "MazeMap.h"
#include "MazeIO.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_TURNS 150

static MazeMap mm;

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

    mm_clear(&mm);
    SET_SQUARE(&mm, mm.loc.r, mm.loc.c, PRESENT);

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

        write_output("T");
    }
    return 0;
}
