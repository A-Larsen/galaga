LIBS = ./libs/SDL2.lib ./libs/SDL2_image.lib ./libs/SDL2_ttf.lib

PROG = galaga

build: $(PROG).c
	gcc -g -o $(PROG) $(PROG).c $(LIBS)

clean:
	rm -rf $(PROG)

.PHONY: clean
