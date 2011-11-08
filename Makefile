CXXFLAGS=-g -O0 -Wall -Iglm-0.9.2.7
LDFLAGS=-lSDL -lGL -lGLEW  -lsfml-audio

all: ssb

ssb: main.o glutils.o util.o Fighter.o audio.o explosion.o FrameManager.o
	g++ $(CXXFLAGS) $(LDFLAGS) -o $@ $^

clean:
	rm -f main.o ssb glutils.o Fighter.o util.o audio.o explosion.o FrameManager.o
