CXXFLAGS=-g -O0 -Wall -Iglm-0.9.2.7
LDFLAGS=-lSDL -lGL -lGLEW  -lsfml-audio

all: ssb

ssb: main.o glutils.o util.o Fighter.o audio.o explosion.o FrameManager.o StatsManager.o \
	Attack.o FighterState.o GameEntity.o Projectile.o CameraManager.o
	g++ $(CXXFLAGS) $(LDFLAGS) -o $@ $^

clean:
	rm -f ssb *.o
