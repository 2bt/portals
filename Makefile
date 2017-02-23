CF = -Wall --std=c++14 -O3
LF = -Wall --std=c++14 -lSDL2 -lSDL2_image -lGLEW -lGL

CXX = g++
SRC = $(wildcard src/*.cpp)
OBJ = $(SRC:src/%.cpp=obj/%.o)
TRG = portals

all: $(TRG)


obj/%.o: src/%.cpp Makefile
	@mkdir -p obj/
	$(CXX) $(CF) $< -c -o $@


$(TRG): $(OBJ) Makefile
	$(CXX) $(OBJ) -o $@ $(LF)


clean:
	rm -rf obj/ $(TRG)

