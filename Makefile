KISS_PARTICLES=kiss-particles

CXXFLAGS=-g -O0 -Wall -Iglm-0.9.2.7 -I$(KISS_PARTICLES) -IirrKlang-1.3.0/include -m32
LDFLAGS=-lSDL -lGL -lGLEW -lIrrKlang -LirrKlang-1.3.0/bin/linux-gcc -lpthread

all: ssb

ssb: main.o Engine.o util.o Fighter.o AudioManager.o ExplosionManager.o \
	FrameManager.o StatsManager.o Attack.o FighterState.o GameEntity.o \
	Projectile.o CameraManager.o StageManager.o FontManager.o Controller.o \
	libkiss_particles.a InGameState.o MenuState.o StatsGameState.o ParamReader.o \
	MatrixStack.o Player.o
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
