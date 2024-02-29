LIBS = -lSDL2 -lSDL2_image -lSDL2_ttf -lm

PROG = galaga

build: $(PROG).c
	gcc -g -o $(PROG) $(PROG).c $(LIBS)

clean:
	rm -rf $(PROG)

.PHONY: clean
