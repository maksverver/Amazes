CFLAGS=-Wall -Wextra -O -g -ansi
OBJS=MazeMap.o
PLAYER_OBJS=player.o $(OBJS)

all: player

player: $(PLAYER_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(PLAYER_OBJS)

clean:
	rm -f $(PLAYER_OBJS)

distclean: clean
	rm -f player

.PHONY: all clean distclean
