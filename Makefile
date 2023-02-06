.POSIX:
CC      = gcc
CCDEBUG = -ggdb
CFLAGS  = -Wall -Wextra -Os 
LDFLAGS =
LDLIBS  = -lm -lSDL2 -lSDL2_image -lSDL2_mixer -lSDL2_ttf -lX11
#-lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

obj = display.o ship.o list.o main.o 

asteroid: $(obj)
	$(CC) $(CCDEBUG) $(LDFLAGS) -o $@ $(obj) $(LDLIBS)

clean:
	rm -f asteroid $(obj)

list.o: list.c list.h
main.o: main.c list.h
ship.o: ship.c ship.h
display.o: display.c display.h

.PHONY: test
