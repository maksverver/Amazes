#ifndef MAZE_MAP_H
#define MAZE_MAP_H

#include <stdbool.h>
#include <stdio.h>

/* Values for squares and walls: */
#define UNKNOWN  0
#define PRESENT +1
#define ABSENT  -1

#define WIDTH  (25)
#define HEIGHT (25)

typedef enum Dir     { NORTH = 0, EAST = 1, SOUTH = 2, WEST = 3 } Dir;
typedef enum RelDir  { FRONT = 0, RIGHT = 1, BACK = 2, LEFT = 3 } RelDir;

#define TURN(dir, rel_dir) ((Dir)(((int)dir + (int)rel_dir + 4)%4))
#define DR(dir) (dir_dr[(int)dir])
#define DC(dir) (dir_dc[(int)dir])
#define RDR(r, dir) ((r + DR(dir) + HEIGHT)%HEIGHT)
#define CDC(c, dir) ((c + DC(dir) + WIDTH)%WIDTH)

#define SQUARE(mm, r, c)        ((mm)->grid[r][c].square)
#define SET_SQUARE(mm, r, c, v) ((void)((mm)->grid[r][c].square = v))

#define WALL(mm, r, c, dir)         (mm_get_wall(mm, r, c, dir))
#define SET_WALL(mm, r, c, dir, v)  ((void)mm_set_wall(mm, r, c, dir, v))

typedef struct MazeCell
{
    signed char square : 2, wall_w : 2, wall_n : 2;
} MazeCell;

typedef struct Point
{
    int r, c;
} Point;

typedef struct Rect
{
    int top, right, bottom, left;
} Rect;

typedef struct MazeMap
{
    MazeCell    grid[WIDTH][HEIGHT];
    Point       loc;
    Dir         dir;
    Rect        border;
    bool        bound_ns, bound_ew;
} MazeMap;

extern int dir_dr[4], dir_dc[4];

extern void mm_clear(MazeMap *mm);
extern void mm_clear_squares(MazeMap *mm);
extern void mm_initialize(MazeMap *mm, int r, int c, Dir dir);
extern void mm_look(MazeMap *mm, const char *line, RelDir rel_dir);
extern void mm_infer(MazeMap *mm);
extern void mm_move(MazeMap *mm, char move);
extern void mm_turn(MazeMap *mm, const char *move);
extern int  mm_get_wall(MazeMap *mm, int r, int c, Dir dir);
extern void mm_set_wall(MazeMap *mm, int r, int c, Dir dir, int val);
extern int  mm_count_squares(MazeMap *mm);

#endif /* ndef MAZE_MAP_H */
