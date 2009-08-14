#ifndef MAZE_IO_INCLUDED
#define MAZE_IO_INCLUDED

#include "MazeMap.h"

extern void mm_print(MazeMap *mm, FILE *fp);
extern const char *mm_encode(MazeMap *mm);

#endif /* ndef MAZE_IO_INCLUDED */
