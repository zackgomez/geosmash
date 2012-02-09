KISS_PARTICLES=kiss-particles
OBJDIR=obj
SRCDIR=src

CXX=g++
CXXFLAGS=-g -O0 -Wall -Iglm-0.9.2.7 -I$(KISS_PARTICLES) -IirrKlang-1.3.0/include -m32
LDFLAGS=-lSDL -lGL -lGLEW -lIrrKlang -LirrKlang-1.3.0/bin/linux-gcc -lpthread

OBJECTS = $(patsubst $(SRCDIR)/%,$(OBJDIR)/%,$(patsubst %.cpp,%.o,$(wildcard $(SRCDIR)/*.cpp)))

all: ssb

ssb: $(OBJECTS) obj/libkiss_particles.a
	$(CXX) $(CXXFLAGS)  -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp obj
	$(CXX) -c $(CXXFLAGS) -o $(OBJDIR)/$*.o $<

obj/libkiss_particles.a: force_look
	cd $(KISS_PARTICLES) && $(MAKE) libkiss_particles.a
	cp $(KISS_PARTICLES)/libkiss_particles.a obj/

clean:
	rm -f ssb obj/*
	rmdir obj
	cd $(KISS_PARTICLES) && make clean

debug: ssb
	./ssb --debug

force_look:
	true

obj:
	mkdir obj

.PHONY: clean force_look
