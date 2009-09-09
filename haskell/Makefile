all: player

player: player.hs
	ghc -O $< -o $@

clean:
	rm -f *.o *.hi

distclean: clean
	rm -f player

.PHONY: all clean distclean
