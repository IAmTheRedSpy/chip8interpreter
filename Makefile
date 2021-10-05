CXXFLAGS = $(shell fltk-config --use-gl --use-images --cxxflags ) -I. -fext-numeric-literals
LDFLAGS  = $(shell fltk-config --use-gl --use-images --ldflags )
LDSTATIC = $(shell fltk-config --use-gl --use-images --ldstaticflags )

all:
	g++-10 chip8interpreter.cpp -std=c++20 -o chip $(CXXFLAGS) $(LDFLAGS) $(LDSTATIC)

clean:
	rm chip
