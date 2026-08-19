# F3DEngine — macOS / Linux build
# Windows: use compile.bat

CXX      ?= g++
CXXFLAGS  = -std=c++23 -O2 -Wall
TARGET    = f3dengine
SRC       = main.cpp
JSON_HDR  = third_party/nlohmann/json.hpp
JSON_URL  = https://raw.githubusercontent.com/nlohmann/json/v3.11.3/single_include/nlohmann/json.hpp

INCLUDES  = -I. -Ithird_party
LDFLAGS   =
LIBS      =

UNAME := $(shell uname -s)

ifeq ($(UNAME),Darwin)
  BREW_PREFIX := $(shell brew --prefix 2>/dev/null || echo /opt/homebrew)
  INCLUDES += -I$(BREW_PREFIX)/include -D_THREAD_SAFE
  INCLUDES += -I$(BREW_PREFIX)/opt/libomp/include
  LDFLAGS  += -L$(BREW_PREFIX)/lib -L$(BREW_PREFIX)/opt/libomp/lib
  LIBS     += -lSDL2 -framework OpenGL -lomp
endif

ifeq ($(UNAME),Linux)
  INCLUDES += $(shell sdl2-config --cflags 2>/dev/null)
  LDFLAGS  += $(shell sdl2-config --libs 2>/dev/null)
  LIBS     += -lGL -fopenmp
endif

.PHONY: all clean run deps snapshot

all: $(JSON_HDR) $(TARGET)

$(TARGET): $(SRC) $(JSON_HDR)
	$(CXX) $(SRC) -o $(TARGET) $(CXXFLAGS) $(INCLUDES) $(LDFLAGS) $(LIBS)

$(JSON_HDR):
	@mkdir -p third_party/nlohmann
	curl -fsSL -o $(JSON_HDR) $(JSON_URL)

deps: $(JSON_HDR)

run: $(TARGET)
	./$(TARGET)

snapshot: $(TARGET)
	./scripts/render_snapshot.sh build/snapshot.png

clean:
	rm -f $(TARGET)
