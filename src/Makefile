CFLAGS=-Wall -Wextra -O0 -g -ansi
OBJS=MazeMap.o MazeIO.o
PLAYER_OBJS=player.o $(OBJS)
CONVERT_OBJS=convert.o $(OBJS)

all: player convert

player: 	$(PLAYER_OBJS);		$(CC) $(LDFLAGS) -o $@ $(PLAYER_OBJS)
convert: 	$(CONVERT_OBJS);	$(CC) $(LDFLAGS) -o $@ $(CONVERT_OBJS)

clean:
	rm -f $(PLAYER_OBJS) $(CONVERT_OBJS)

distclean: clean
	rm -f player convert

.PHONY: all clean distclean
