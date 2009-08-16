#define _POSIX_SOURCE
#include "MazeMap.h"
#include "MazeIO.h"
#include <assert.h>
#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_ARGS 50

typedef struct Score
{
    int moves, sq_disc, sq_disc_first, captures;
} Score;

/* Global data:

   The master maze map keeps track of existing walls and squares discovered
   (by either player). Initially, all squares are unknown, and all wals are
   known (either present or absent). This is used to determine valid moves and
   keep track of which player discovers squares first.

   For each player, there is a maze map that keeps track of information this
   player has; this information is used to determine how many squares each
   player has discovered.

   For each player, we have a Score struct that keeps track of the total
   score, consisting of:
    - number of moves requested (-1 point each)
    - number of squares discovered (1 point each)
    - number of squares discovered first (1 additional point each)
    - number of times the opponent was captured (100 points each)
   TODO/FIXME: no support for sudden death yet.
*/

static MazeMap mm_master, mm_player[2];
static Score score[2];
static FILE *fpr[2], *fpw[2];
static int pid[2];

/* Determines what `player' can see in the given direction. */
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
    mm_clear_squares(&mm_master);
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
    } while (WALL(&mm_master, mm->loc.r, mm->loc.c, back) != ABSENT);
}

static void player_looks(int player)
{
    static const Dir look_dirs[4] = { FRONT, RIGHT, BACK, LEFT };

    int d;

    for (d = 0; d < 4; ++d)
    {
        Dir dir = look_dirs[d];
        const char *line = line_of_sight(player, dir);
        mm_look(&mm_player[player], line, dir);
        write_player(0, line);
    }

    /* distance to opponent -- TODO later */
    write_player(0, "-1");
}

/* Checks the syntax of the turn for syntactic validity.

   A turn is syntactically valid if it consists only of letters F, T, L and R,
   and is between 1 and 256 (inclusive) letters long. */
static bool is_valid_turn(const char *s)
{
    const char *p;
    for (p = s; *p; ++p) if (strchr("FTLR", *p) == NULL) return false;
    return p > s && p <= s + 256;
}

/* Computes the length of the maximal prefix of `turn' that constitues a valid
   turn for `player', assuming the turn is syntactically valid. A turn is valid
   if all moves in the turn can be performed in order without walking through
   walls. */
static int valid_turn_size(const char *turn, int r, int c, Dir dir)
{
    int i;

    for (i = 0; turn[i]; ++i)
    {
        switch (turn[i])
        {
        case 'F': dir = TURN(dir, FRONT); break;
        case 'T': dir = TURN(dir, BACK);  break;
        case 'L': dir = TURN(dir, LEFT);  break;
        case 'R': dir = TURN(dir, RIGHT); break;
        default: assert(0);
        }
        if (WALL(&mm_master, r, c, dir) != ABSENT) break;
        r = RDR(r, dir);
        c = CDC(c, dir);
    }

    return i;
}

static bool player_moves(int player, const char *turn)
{
    MazeMap *mm = &mm_player[player];
    Point old_loc = mm->loc;
    int len;

    if (turn == NULL || !is_valid_turn(turn))
    {
        printf("Player %d made an invalid move: `%s'!\n",
                player + 1, turn == NULL ? "<eof>" : turn);
        return false;
    }

    /* Perform valid moves */
    len = valid_turn_size(turn, mm->loc.r, mm->loc.c, mm->dir);
    if (len < (int)strlen(turn))
    {
        printf("WARNING: Player %d's turn (`%s') was truncated to %d moves.\n",
               player + 1, turn, len);
    }
    for (; len > 0; --len) mm_move(mm, *turn++);

    /* If we end up at our starting point, do an extra T move */
    if (mm->loc.r == old_loc.r && mm->loc.c == old_loc.c)
    {
        printf("WARNING: Player %d returned to his original location.\n",
               player + 1);
        mm_move(mm, 'T');
    }

    return true;
}

static int total_score(const Score *sc)
{
    return -sc->moves + sc->sq_disc + sc->sq_disc_first + 100*sc->captures;
}

/* FIXME: is starting square initially discovered or not?
          if so, call player_scores at start of game before any turns have
          been made. */

static void player_scores(int t, int player, const char *turn)
{
    int r, c;
    MazeMap *mm = &mm_player[player];
    Score *sc = &score[player];
    Score new_score = score[player];

    new_score.moves          = sc->moves + strlen(turn);
    new_score.captures       = sc->captures;   /* TODO! */
    new_score.sq_disc        = 0;
    new_score.sq_disc_first  = sc->sq_disc_first;

    /* Count discovered squares */
    new_score.sq_disc = 0;
    for (r = 0; r < HEIGHT; ++r)
    {
        for (c = 0; c < WIDTH; ++c)
        {
            if (SQUARE(mm, r, c) == PRESENT)
            {
                ++new_score.sq_disc;
                if (SQUARE(&mm_master, r, c) == UNKNOWN)
                {
                    SET_SQUARE(&mm_master, r, c, PRESENT);
                    ++new_score.sq_disc_first;
                }
            }
        }
    }

    printf(" %5d %5d %5d %5d %5d %5d %5d %5d\n", t + 1, player + 1,
           new_score.moves - sc->moves,
           new_score.sq_disc - sc->sq_disc,
           new_score.sq_disc_first - sc->sq_disc_first,
           new_score.captures - sc->captures,
           total_score(&new_score) - total_score(sc),
           total_score(&new_score));

    *sc = new_score;
}

static int final_score(int player)
{
    int res = total_score(&score[player]);
    if (res < 0) res = 0;
    if (res > 1000) res = 1000;
    return res;
}

static void disable_sigpipe()
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa, NULL);
}

static void initialize(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("usage:\n"
               "  judge <maze file> <player1 command>\n");
        exit(EXIT_FAILURE);
    }

    disable_sigpipe();

    load_maze(argv[1]);

    mm_clear(&mm_player[0]);
    place_player(&mm_player[0]);
    launch(argv[2], &fpr[0], &fpw[0], &pid[0]);
}

static void finalize()
{
    write_player(0, "Quit");
}

int main(int argc, char *argv[])
{
    int t;
    initialize(argc, argv);

    printf("Turn  Player Moves Disc. First Capt. Score Total\n");
    printf("------------------------------------------------\n");

    for (t = 0; t < 300; ++t)
    {
        int p = t%2;
        const char *turn;
        if (t == 0) write_player(p, "Start");
        if (p == 1) continue;  /* player 2 -- TODO later */
        player_looks(p);
        turn = read_player(p);
        if (!player_moves(p, turn)) break;
        mm_infer(&mm_player[p]);
        player_scores(t, p, turn);
    }
    printf("------------------------------------------------\n");
    finalize();
    printf("Score: %d - %d\n", final_score(0), final_score(1)); 
    return 0;
}
