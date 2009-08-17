CFLAGS=-Wall -Wextra -O0 -g -ansi
LDFLAGS=-lm -g

SUBMISSION_SRC=MazeMap.c MazeIO.c player.c

OBJS=MazeMap.o MazeIO.o
PLAYER_OBJS=player.o $(OBJS)
CONVERT_OBJS=convert.o $(OBJS)
ARBITER_OBJS=arbiter.o $(OBJS)

TARGETS=player convert arbiter submission.c

all: $(TARGETS)

player: 	$(PLAYER_OBJS);		$(CC) $(LDFLAGS) -o $@ $(PLAYER_OBJS)
convert: 	$(CONVERT_OBJS);	$(CC) $(LDFLAGS) -o $@ $(CONVERT_OBJS)
arbiter:  	$(ARBITER_OBJS);  	$(CC) $(LDFLAGS) -o $@ $(ARBITER_OBJS)

submission.c: $(SUBMISSION_SRC)
	../tools/compile.pl $(SUBMISSION_SRC) >$@
	
clean:
	rm -f $(PLAYER_OBJS) $(CONVERT_OBJS) $(ARBITER_OBJS)

distclean: clean
	rm -f $(TARGETS)

.PHONY: all clean distclean
