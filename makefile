CXX       = g++
CXXFLAGS  = -c -std=c++14
LDFLAGS   = -lsfml-graphics -lsfml-window -lsfml-system
BDIR      = build
VPATH     = src src/objects src/tools
SOURCES   = main.cpp \
	    board.cpp cell.cpp food.cpp entity.cpp \
            tools.cpp
OBJECTS   = $(SOURCES:%.cpp=$(BDIR)/%.o)
EXECUTABLE= $(BDIR)/life

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@  $(LDFLAGS)

$(BDIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@
