LIBS = -lSDL2 -lSDL2_image -lSDL2_ttf -lm

PROG = galaga

game: game.c $(PROG).c
	gcc -g -o $(PROG) game.c $(PROG).c $(LIBS)

tests: game.c tests.c
	gcc -g -o tests game.c tests.c $(LIBS)

clean:
	rm -rf $(PROG) tests

.PHONY: clean
