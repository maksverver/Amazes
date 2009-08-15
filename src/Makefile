CFLAGS=-Wall -Wextra -O0 -g -ansi
LDFLAGS=-lm -g
OBJS=MazeMap.o MazeIO.o
PLAYER_OBJS=player.o $(OBJS)
CONVERT_OBJS=convert.o $(OBJS)
JUDGE_OBJS=judge.o $(OBJS)

all: player convert judge

player: 	$(PLAYER_OBJS);		$(CC) $(LDFLAGS) -o $@ $(PLAYER_OBJS)
convert: 	$(CONVERT_OBJS);	$(CC) $(LDFLAGS) -o $@ $(CONVERT_OBJS)
judge:  	$(JUDGE_OBJS);  	$(CC) $(LDFLAGS) -o $@ $(JUDGE_OBJS)

clean:
	rm -f $(PLAYER_OBJS) $(CONVERT_OBJS) $(JUDGE_OBJS)

distclean: clean
	rm -f player convert

.PHONY: all clean distclean
