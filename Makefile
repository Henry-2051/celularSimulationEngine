# to compile and run : make run
#
# CXX_FLAGS := -std=c++23 -pedantic-errors -Wall -Weffc++ -Wextra -Wconversion -Wsign-conversion -O3

# define compiler to use
CXX    := g++
OUTPUT := sfmlgame
OS     := $(shell uname)

# linux compiler / linker flags
ifeq ($(OS), Linux)
    CXX_FLAGS := -std=c++23 -pedantic-errors -Wall -O2
    INCLUDES  := -I./src -I ./src/imgui -I ./src/imgui-sfml
    LDFLAGS   := -O3 -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio -lGL
endif

# the source files for the ecs game engine
SRC_FILES := $(wildcard src/*.cpp src/imgui/*.cpp src/imgui-sfml/*.cpp)
OBJ_FILES := $(SRC_FILES:.cpp=.o)

# include dependency files
DEP_FILES := $(SRC_FILES:.cpp=.d)
-include $(DEP_FILES)

# all of these targets will be made if you just type make
all: $(OUTPUT)

# define the main executable requirements / command
$(OUTPUT): $(OBJ_FILES) Makefile
	$(CXX) $(OBJ_FILES) $(LDFLAGS) -o ./bin/$@

# specifies how the object files are compiled from cpp files
.cpp.o:
	$(CXX) -MMD -MP -c $(CXX_FLAGS) $(INCLUDES) $< -o $@

# typing make clean with remove all intermediate files
clean:
	rm -f $(OBJ_FILES) $(DEP_FILES) ./bin/$(OUTPUT)

# typing make run will compile and run the program
run: $(OUTPUT)
	cd bin && ./$(OUTPUT) && cd ..
