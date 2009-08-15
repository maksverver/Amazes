#define _POSIX_SOURCE
#include "MazeMap.h"
#include "MazeIO.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>

#define MAX_ARGS 3

static MazeMap mm_master, mm_player[2];
static FILE *fpr[2], *fpw[2];
static int pid[2];

static char *line_of_sight(int player, RelDir rel_dir)
{
    static char buf[1 + (WIDTH > HEIGHT) ? WIDTH : HEIGHT];

    char *p = buf;
    int r, c, front, left, right;

    r = mm_player[player].loc.r;
    c = mm_player[player].loc.c;
    front = TURN(mm_player[player].dir, rel_dir);
    left  = TURN(front, LEFT);
    right = TURN(front, RIGHT);

    while (WALL(&mm_master, r, c, front) == ABSENT)
    {
        bool open_left, open_right;
        r = RDR(r, front);
        c = CDC(c, front);
        open_left  = WALL(&mm_master, r, c, left)  == ABSENT;
        open_right = WALL(&mm_master, r, c, right) == ABSENT;
        if (open_left && open_right)
            *p++ = 'B';
        else
        if (open_left && !open_right)
            *p++ = 'L';
        else
        if (!open_left && open_right)
            *p++ = 'R';
        else  /* (!open_left && !open_right) */
            *p++ = 'N';
    }
    *p++ = 'W';
    *p   = '\0';
    return buf;
}

static char * const *parse_args(char *command)
{
    static char *argv[MAX_ARGS + 2];
    int i;
    for (i = 0; ; ++i)
    {
        if (i == MAX_ARGS + 2)
        {
            printf("Too many arguments in command (limit is %d)\n", MAX_ARGS);
            exit(EXIT_FAILURE);
        }
        argv[i] = strtok((i == 0) ? command : NULL, " \t\v\r\n");
        if (argv[i] == NULL) break;
    }
    return argv;
}

static void launch(char *command, FILE **fpr, FILE **fpw, int *pid)
{
    int fd[2][2];
    if (pipe(fd[0]) != 0 || pipe(fd[1]) != 0)
    {
        fprintf(stderr, "launch(): couldn't create pipes!\n");
        exit(EXIT_FAILURE);
    }
    if ((*pid = fork()) == -1)
    {
        fprintf(stderr, "launch(): couldn't fork!\n");
        exit(EXIT_FAILURE);
    }

    if (*pid == 0)  /* in child */
    {
        char * const *argv = parse_args(command);
        dup2(fd[0][0], 0);
        dup2(fd[1][1], 1);
        close(fd[0][0]);
        close(fd[0][1]);
        close(fd[1][0]);
        close(fd[1][1]);
        execv(argv[0], argv);
        /* print to stderr because original stdout was closed */
        fprintf(stderr, "Couldn't execute `%s'!\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    else  /* in parent */
    {
        close(fd[0][0]);
        close(fd[1][1]);
        *fpr = fdopen(fd[1][0], "rt");
        *fpw = fdopen(fd[0][1], "wt");
        if (*fpr == NULL || *fpw == NULL)
        {
            printf("launch(): couldn't fdopen pipes!");
            exit(EXIT_FAILURE);
        }
    }
}

static void load_maze(const char *path)
{
    FILE *fp = fopen(path, "rt");
    if (fp == NULL || !mm_scan(&mm_master, fp))
    {
        printf("Couldn't load maze from `%s'!\n", path);
        exit(EXIT_FAILURE);
    }
    fclose(fp);
}

static void write_player(int p, const char *msg)
{
    fprintf(fpw[p], "%s\n", msg);
    fflush(fpw[p]);
}

static char *read_player(int p)
{
    static char buf[1024];
    char *eol;

    if (fgets(buf, sizeof(buf), fpr[p]) == NULL) return NULL;
    if ((eol = strchr(buf, '\n')) == NULL) return NULL;
    while (eol > buf && isspace(*(eol - 1))) --eol;
    *eol = '\0';
    return buf;
}

static void place_player(MazeMap *mm)
{
    int back;
    do {
        mm->loc.r = rand()%HEIGHT;
        mm->loc.c = rand()%WIDTH;
        mm->dir   = (Dir)(rand()%4);
        back = TURN(mm->dir, BACK);
    } while (WALL(mm, mm->loc.r, mm->loc.c, back) != ABSENT);
}

/* Checks the syntax of the move for validity. */
static bool is_valid_turn(const char *s)
{
    const char *p;
    for (p = s; *p; ++p) if (strchr("FTLR", *p) == NULL) return false;
    return p - s <= 256;
}


int main(int argc, char *argv[])
{
    int turn;
    struct sigaction sa;

    /* Ignore SIGPIPE */
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa, NULL);

    if (argc != 3)
    {
        printf("usage:\n"
               "  judge <maze file> <player1 command>\n");
        return 1;
    }

    load_maze(argv[1]);
    memcpy(&mm_player[0], &mm_master, sizeof(MazeMap));
    place_player(&mm_player[0]);
    launch(argv[2], &fpr[0], &fpw[0], &pid[0]);

    for (turn = 0; turn < 300; ++turn)
    {
        int p = turn%2;
        char *turn;

        if (turn == 0) write_player(p, "Start");

        if (p == 1) continue;  /* player 2 -- TODO later */

        write_player(0, line_of_sight(p, FRONT));
        write_player(0, line_of_sight(p, RIGHT));
        write_player(0, line_of_sight(p, BACK));
        write_player(0, line_of_sight(p, LEFT));
        write_player(0, "-1");  /* distance to opponent -- TODO later */

        turn = read_player(p);
        if (turn == NULL || !is_valid_turn(turn))
        {
            printf("Player %d made an invalid move: `%s'!\n",
                   p + 1, turn == NULL ? "<eof>" : turn);
            break;
        }

        /* TODO:
            1. truncate move to valid parts only (don't walk through walls)
            2. if move ends at same square it begun, do extra T
            3. infer newly discovered squares (LATER: keep score)
        */
        mm_move(&mm_player[p], turn);
    }

    write_player(0, "Quit");
    return 0;
}
