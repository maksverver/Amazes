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
#include <fcntl.h>
#include <sys/wait.h>

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
*/

static const char *arg_csv;
static int num_players;
static MazeMap mm_master, mm_player[2];
static bool map_complete[2];
static Score score[2];
static FILE *fpr[2], *fpw[2], *fpe[2], *fp_csv;
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

static void launch(char *command, FILE **fpr, FILE **fpw, FILE **fpe, int *pid)
{
    int fd[3][2];
    if (pipe(fd[0]) != 0 || pipe(fd[1]) != 0 || pipe(fd[2]) != 0)
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
        dup2(fd[2][1], 2);
        close(fd[0][0]);
        close(fd[0][1]);
        close(fd[1][0]);
        close(fd[1][1]);
        close(fd[2][0]);
        close(fd[2][1]);
        execv(argv[0], argv);
        /* print to stderr because original stdout was closed */
        fprintf(stderr, "Couldn't execute `%s'!\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    else  /* in parent */
    {
        close(fd[0][0]);
        close(fd[1][1]);
        close(fd[2][1]);
        /* Make reading from child's error stream non-blocking: */
        fcntl(fd[2][0], F_SETFL, fcntl(fd[2][0], F_GETFL)|O_NONBLOCK);
        *fpw = fdopen(fd[0][1], "wt");  /* stdin */
        *fpr = fdopen(fd[1][0], "rt");  /* stdout */
        *fpe = fdopen(fd[2][0], "rt");  /* stderr */
        if (*fpr == NULL || *fpw == NULL || *fpe == NULL)
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

    /* Flush output so it is visible in case the fgets() blocks */
    fflush(stdout);
    fflush(stderr);
    if (fgets(buf, sizeof(buf), fpr[p]) == NULL) return NULL;
    if ((eol = strchr(buf, '\n')) == NULL) return NULL;
    while (eol > buf && isspace(*(eol - 1))) --eol;
    *eol = '\0';
    return buf;
}

char *read_comments(FILE *fp)
{
    char *p, *q;
    static char comment_tmp[2048];
    static char comment_buf[2*sizeof(comment_tmp) + 2];

    /* Read a bunch of data from stderr */
    comment_tmp[fread(comment_tmp, 1, sizeof(comment_tmp) - 1, fp)] = '\0';


    /* Copy to comment_buf, while adding double-quote characters in front and
       back and doubling all exitsing double-quote characters in comment_tmp. */
    q = comment_buf;
    *q++ = '"';
    for (p = comment_tmp; *p != '\0' && (p[0] != '\n' || p[1] != '\0'); ++p)
    {
        if (*p == '"') *q++ = *p;
        *q++ = *p;
    }
    *q++ = '"';
    *q   = '\0';
    return comment_buf;
}

/* Returns the squared Euclidian distance between two players */
static int player_dist()
{
    int dr, dc;
    if (num_players < 2) return -1;
    dr = mm_player[0].loc.r - mm_player[1].loc.r;
    dc = mm_player[0].loc.c - mm_player[1].loc.c;
    return dr*dr + dc*dc;
}

static void place_player(MazeMap *mm)
{
    int r, c, dir;
    do {
        r   = rand()%HEIGHT;
        c   = rand()%WIDTH;
        dir = (Dir)(rand()%4);
    } while (WALL(&mm_master, r, c, TURN(dir, BACK)) != ABSENT);
    mm_initialize(mm, r, c, dir);
}

static void player_looks(int player)
{
    static const Dir look_dirs[4] = { FRONT, RIGHT, BACK, LEFT };

    int d;
    char buf[12];

    /* Write four lines of sight */
    for (d = 0; d < 4; ++d)
    {
        Dir dir = look_dirs[d];
        const char *line = line_of_sight(player, dir);
        mm_look(&mm_player[player], line, dir);
        write_player(player, line);
    }

    /* Infer other parts of the maze */
    mm_infer(&mm_player[player]);

    /* Write distance from opponent */
    extern int snprintf(char *str, size_t size, const char *format, ...);
    snprintf(buf, sizeof(buf), "%d", player_dist());
    write_player(player, buf);
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

    if (!is_valid_turn(turn))
    {
        printf("Player %d made an invalid move: `%s'!\n", player + 1, turn);
        return false;
    }

    /* Perform valid moves */
    len = valid_turn_size(turn, mm->loc.r, mm->loc.c, mm->dir);
    if (len < (int)strlen(turn))
    {
        printf("WARNING: Player %d's turn (`%s') was truncated by %d moves.\n",
               player + 1, turn, (int)strlen(turn) - len);
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

static Score player_scores(int player, const char *turn)
{
    int r, c;
    MazeMap *mm = &mm_player[player];
    const Score *sc = &score[player];
    Score new_score = score[player];

    new_score.moves          = sc->moves + strlen(turn);
    new_score.captures       = sc->captures;
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

    if (new_score.sq_disc == HEIGHT*WIDTH)
    {
        map_complete[player] = true;
    }
    else
    {
        if (player_dist() == 0)
            ++new_score.captures;
    }

    return new_score;
}

static void log_progress( int turn_no, int player,
                          const char *turn, Score *new_score,
                          const char *comments )
{
    const Score *old_score = &score[player];
    const int moves      = new_score->moves - old_score->moves;
    const int discovered = new_score->sq_disc - old_score->sq_disc;
    const int first      = new_score->sq_disc_first - old_score->sq_disc_first;
    const int captures   = new_score->captures - old_score->captures;
    const int total      = total_score(new_score);
    const int score      = total - total_score(old_score);
    const char *map_desc = mm_encode(&mm_player[player], true);

    printf(" %5d %5d %5d %5d %5d %5d %5d %5d\n", turn_no + 1, player + 1,
            moves, discovered, first, captures, score, total );

    if (fp_csv != NULL)
    {
        fprintf(fp_csv, "%d,%d,%d,%d,%d,%d,%d,%d,%s,%s,%s\n",
                        turn_no + 1, player + 1,
                        moves, discovered, first, captures, score, total,
                        turn, map_desc, comments );
    }
}


static int final_score(int player, int winner)
{
    int res = total_score(&score[player]);
    if (winner != -1) res = (player == winner) ? 2*res : 0;
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

static int parse_options(int argc, char *argv[])
{
    int i, j;
    for (i = j = 1; i < argc; ++i)
    {
        if (memcmp(argv[i], "--csv=", 6) == 0)
            arg_csv = argv[i] + 6;
        else
        if (strcmp(argv[i], "--csv") == 0 && ++i < argc)
            arg_csv = argv[i];
        else
        if (memcmp(argv[i], "--seed=", 7) == 0)
            srand(atoi(argv[i] + 7));
        else
        if (strcmp(argv[i], "--seed") == 0 && ++i < argc)
            srand(atoi(argv[i]));
        else
            argv[j++] = argv[i];
    }
    return j;
}

static void open_csv(const char *path)
{
    if ((fp_csv = fopen(arg_csv, "wt")) == NULL)
    {
        printf("Couldn't open CSV file `%s'!\n", path);
        exit(EXIT_FAILURE);
    }
    fprintf(fp_csv, "TurnNo,Player,Moves,Discovered,First,Captures,Score,"
                    "Total,Turn,Map,Comments\n");
}

static void initialize(int argc, char *argv[])
{
    int p;

    argc = parse_options(argc, argv);

    if (argc < 3 || argc > 4)
    {
        printf(
"usage:\n"
"\tarbiter [options] <maze file> <player 1 command> [<player 2 command>]\n"
"options:\n"
"\t--csv <file>\n"
"\t--seed <value>\n");
        exit(EXIT_FAILURE);
    }
    if (arg_csv != NULL) open_csv(arg_csv);

    load_maze(argv[1]);
    num_players = argc - 2;
    do {
        for (p = 0; p < num_players; ++p)
            place_player(&mm_player[p]);
    } while (num_players > 1 && player_dist() < 17*17);

    /* Start player programs: */
    disable_sigpipe();
    for (p = 0; p < num_players; ++p)
        launch(argv[2 + p], &fpr[p], &fpw[p], &fpe[p], &pid[p]);
}

static void finalize()
{
    int p;
    for (p = 0; p < num_players; ++p)
        write_player(p, "Quit");
    for (p = 0; p < num_players; ++p)
        waitpid(pid[p], NULL, 0);
}

int main(int argc, char *argv[])
{
    int t, p;
    initialize(argc, argv);

    printf("#Turn Player Moves Disc. First Capt. Score Total\n");
    printf("------------------------------------------------\n");

    /* Discover starting square */
    for (p = 0; p < num_players; ++p)
    {
        Score new_score = player_scores(p, "");
        log_progress(-1, p, "", &new_score, "");
        score[p] = new_score;
    }

    write_player(0, "Start");

    for (t = 0; t < 150*num_players; ++t)
    {
        Score new_score;
        const char *turn;
        const char *comments;

        p = t%num_players;
        player_looks(p);
        if ((turn = read_player(p)) == NULL)
        {
            printf("Unexpected end of input from player %d!\n", p + 1);
            break;
        }
        /*
        mm_print(&mm_player[p], stdout, true);
        printf("(%s)\n", mm_encode(&mm_player[p], true));
        */
        if (!player_moves(p, turn)) break;
        comments = read_comments(fpe[p]);
        new_score = player_scores(p, turn);
        log_progress(t/num_players, p, turn, &new_score, comments);
        score[p] = new_score;
        if (map_complete[p] && (num_players == 1 || player_dist() == 0))
        {
            t++;
            break;
        }
    }
    printf("------------------------------------------------\n");

    if (num_players == 1)
    {
        printf("Score: %d (after %d turns)\n", final_score(0, -1), t);
    }
    else  /* (num_players == 2) */
    {
        if (t == 150*num_players) p = -1;  /* drawn */
        printf("Score: %d - %d (after %d turns)\n", final_score(0, p), final_score(1, p), t);
    }
    finalize();
    return 0;
}
