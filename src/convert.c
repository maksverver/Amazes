#include "MazeIO.h"
#include <string.h>

static MazeMap mm;

int main(int argc, const char *argv[])
{
    if (argc != 2)
    {
        printf("usage:\n"
               "  convert <compact description>\n"
               "  convert <file in human-readable format>\n");
        return 1;
    }

    if (mm_decode(&mm, argv[1]))
    {
        mm_print(&mm, stdout, false);
    }
    else
    {
        if (strcmp(argv[1], "-") == 0)
        {
            if (!mm_scan(&mm, stdin))
            {
                printf("Couldn't read maze map from standard input!\n");
                return 1;
            }
        }
        else
        {
            FILE *fp = fopen(argv[1], "rt");
            if (fp == NULL)
            {
                printf("Argument could neither be decoded "
                       "nor opened as a file!\n");
                return 1;
            }
            if (!mm_scan(&mm, fp))
            {
                fclose(fp);
                printf("Couldn't read maze map from file!\n");
                return 1;
            }
            fclose(fp);
        }
        puts(mm_encode(&mm, false));
    }

    return 0;
}
