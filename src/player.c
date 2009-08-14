#include "MazeMap.h"
#include "MazeIO.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

static char current_line[1024];

MazeMap mm;

const char *next_line()
{
    char *eol;
    if (!fgets(current_line, sizeof(current_line), stdin) ||
        (eol = strchr(current_line, '\n')) != 0)
    {
        fprintf(stderr, "Could not read the next line! Exiting.");
        exit(EXIT_FAILURE);
    }
    return current_line;
}

int main()
{
    mm_clear(&mm);
    printf("%s\n", mm_encode(&mm));
    mm_look(&mm, "W", FRONT);
    mm_look(&mm, "W", RIGHT);
    mm_look(&mm, "LLBNW", BACK);
    mm_look(&mm, "W", LEFT);
    mm_print(&mm, stdout); fputc('\n', stdout);
    printf("%s\n", mm_encode(&mm));
    mm_infer(&mm);
    mm_print(&mm, stdout); fputc('\n', stdout);
    printf("%s\n", mm_encode(&mm));
    mm_move(&mm, "TL");
    mm_print(&mm, stdout); fputc('\n', stdout);
    printf("%s\n", mm_encode(&mm));
    return 0;
}
