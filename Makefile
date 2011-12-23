CXXFLAGS=-g -O0 -Wall -Iglm-0.9.2.7 -Ikiss_particles
LDFLAGS=-lSDL -lGL -lGLEW  -lsfml-audio

all: ssb

ssb: main.o glutils.o util.o Fighter.o audio.o explosion.o FrameManager.o StatsManager.o \
	Attack.o FighterState.o GameEntity.o Projectile.o CameraManager.o \
	StageManager.o FontManager.o Controller.o libkiss_particles.a
	g++ $(CXXFLAGS)  -o $@ $^ $(LDFLAGS)

libkiss_particles.a: force_look
	cd kiss_particles && make libkiss_particles.a
	cp kiss_particles/libkiss_particles.a .

clean:
	rm -f ssb *.o *.a
	cd kiss_particles && make clean

force_look:
	true

.PHONY: clean force_look
