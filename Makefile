KISS_PARTICLES=lib/kiss-particles
KISS_SKELETON=lib/kiss-skeleton
IRRKLANG=lib/irrKlang-1.3.0
GLM=lib/glm-0.9.2.7
OBJDIR=obj
SRCDIR=src

CXX=g++
CXXFLAGS=-g -O0 -Wall -I$(GLM) -I$(KISS_PARTICLES) -I$(IRRKLANG)/include -I$(KISS_SKELETON) -m32
LDFLAGS=-lSDL -lGL -lGLEW -lIrrKlang -L$(IRRKLANG)/bin/linux-gcc -lpthread

OBJECTS = $(patsubst $(SRCDIR)/%,$(OBJDIR)/%,$(patsubst %.cpp,%.o,$(wildcard $(SRCDIR)/*.cpp))) obj/kiss-skeleton.o

all: obj ssb

ssb: $(OBJECTS) obj/libkiss_particles.a
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/kiss-skeleton.o: $(KISS_SKELETON)/kiss-skeleton.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) -c $(CXXFLAGS) -o $(OBJDIR)/$*.o $<

$(OBJDIR)/libkiss_particles.a: force_look
	cd $(KISS_PARTICLES) && $(MAKE) libkiss_particles.a
	cp $(KISS_PARTICLES)/libkiss_particles.a $(OBJDIR)/

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
