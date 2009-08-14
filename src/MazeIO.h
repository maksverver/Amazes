#ifndef MAZE_IO_INCLUDED
#define MAZE_IO_INCLUDED

#include "MazeMap.h"

/* Read/write a map in a human-readable plain-text format: */
extern bool mm_scan(MazeMap *mm, FILE *fp);
extern void mm_print(MazeMap *mm, FILE *fp);

/* Encode/decode map in a compact URL-safe non-human-readable format: */
extern bool mm_decode(MazeMap *mm, const char *desc);
extern const char *mm_encode(MazeMap *mm);

#endif /* ndef MAZE_IO_INCLUDED */
