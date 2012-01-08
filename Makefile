KISS_PARTICLES=kiss-particles

CXXFLAGS=-g -O0 -Wall -Iglm-0.9.2.7 -I$(KISS_PARTICLES) 
LDFLAGS=-lSDL -lGL -lGLEW  -lsfml-audio

all: ssb

ssb: main.o glutils.o util.o Fighter.o AudioManager.o ExplosionManager.o \
	FrameManager.o StatsManager.o Attack.o FighterState.o GameEntity.o \
	Projectile.o CameraManager.o StageManager.o FontManager.o Controller.o \
	libkiss_particles.a InGameState.o MenuState.o StatsGameState.o ParamReader.o
	g++ $(CXXFLAGS)  -o $@ $^ $(LDFLAGS)

libkiss_particles.a: force_look
	cd $(KISS_PARTICLES) && $(MAKE) libkiss_particles.a
	cp $(KISS_PARTICLES)/libkiss_particles.a .

clean:
	rm -f ssb *.o *.a
	cd $(KISS_PARTICLES) && make clean

force_look:
	true

.PHONY: clean force_look
