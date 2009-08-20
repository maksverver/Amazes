CFLAGS=-Wall -Wextra -O0 -g -ansi
LDFLAGS=-lm -g

SUBMISSION_SRC=MazeMap.c MazeIO.c Analysis.c AI.c player.c

OBJS=MazeMap.o MazeIO.o
PLAYER_OBJS=$(OBJS) Analysis.o AI.o player.o
MANUAL_OBJS=$(OBJS) Analysis.o Manual.o player.o
CONVERT_OBJS=$(OBJS) convert.o
ARBITER_OBJS=$(OBJS) arbiter.o

TARGETS=player convert arbiter manual submission.c

all: $(TARGETS)

player: 	$(PLAYER_OBJS);		$(CC) $(LDFLAGS) -o $@ $(PLAYER_OBJS)
convert: 	$(CONVERT_OBJS);	$(CC) $(LDFLAGS) -o $@ $(CONVERT_OBJS)
arbiter:  	$(ARBITER_OBJS);  	$(CC) $(LDFLAGS) -o $@ $(ARBITER_OBJS)


Manual.o: Manual.cpp
	$(CXX) $(CFLAGS) -pthread `fltk-config --cflags` -o $@ -c $<

manual: $(MANUAL_OBJS)
	$(CC) $(LDFLAGS) -pthread `fltk-config --ldflags` -o $@ $(MANUAL_OBJS)

submission.c: $(SUBMISSION_SRC)
	../tools/compile.pl $(SUBMISSION_SRC) >$@
	
clean:
	rm -f *.o

distclean: clean
	rm -f $(TARGETS)

.PHONY: all clean distclean
