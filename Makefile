KISS_PARTICLES=lib/kiss-particles
IRRKLANG=lib/irrKlang-1.3.0
GLM=lib/glm-0.9.2.7
OBJDIR=obj
SRCDIR=src

CXX=g++
CXXFLAGS=-g -O0 -Wall -I$(GLM) -I$(KISS_PARTICLES) -I$(IRRKLANG)/include -m32
LDFLAGS=-lSDL -lGL -lGLEW -lIrrKlang -L$(IRRKLANG)/bin/linux-gcc -lpthread

OBJECTS = $(patsubst $(SRCDIR)/%,$(OBJDIR)/%,$(patsubst %.cpp,%.o,$(wildcard $(SRCDIR)/*.cpp)))

all: obj ssb

ssb: $(OBJECTS) obj/libkiss_particles.a
	$(CXX) $(CXXFLAGS)  -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) -c $(CXXFLAGS) -o $(OBJDIR)/$*.o $<

obj/libkiss_particles.a: force_look
	cd $(KISS_PARTICLES) && $(MAKE) libkiss_particles.a
	cp $(KISS_PARTICLES)/libkiss_particles.a obj/

clean:
	rm -f ssb obj/*
	rm -rf obj/
	cd $(KISS_PARTICLES) && make clean

debug: obj ssb
	./ssb --debug

force_look:
	true

obj:
	mkdir -p obj

.PHONY: clean force_look obj
