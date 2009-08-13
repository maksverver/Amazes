#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

static char current_line[1024];

const char *next_line()
{
    char *eol;
    if (!fgets(current_line, sizeof(current_line), stdin) ||
        (eol = strchr(current_line, '\n')) != 0)
    {
        fprintf(stderr, "Could not read the next line! Exiting.");
        exit(EXIT_FAILURE);
    }
    
}

int main()
{
    
}
