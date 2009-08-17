#ifndef ANALYSIS_H_INCLUDED
#define ANALYSIS_H_INCLUDED

#include "MazeMap.h"

extern void find_distance( const MazeMap *mm, int dist[HEIGHT][WIDTH],
                           int r, int c);

extern const char *construct_turn( const MazeMap *mm, int dist[HEIGHT][WIDTH],
                                   int r1, int c1, int dir1, int r2, int c2);

#endif /* ndef ANALYSIS_H_INCLUDED */
