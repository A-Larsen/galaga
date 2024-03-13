LIBS = -lSDL2 -lSDL2_image -lSDL2_ttf -lm

PROG = galaga

build: game.c $(PROG).c
	gcc -g -o $(PROG) game.c $(PROG).c $(LIBS)

clean:
	rm -rf $(PROG)

.PHONY: clean
