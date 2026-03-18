CXX      = g++
CXXFLAGS = -std=c++11 -Wall -O2 -Wno-unused-result -Wno-stringop-truncation \
           -Igui/include \
           -Iimgui \
           $(shell pkg-config --cflags sdl3 2>/dev/null || sdl3-config --cflags)

LDFLAGS  = -L/usr/local/lib -lSDL3 -lGL -ldl -lssl -lcrypto \
           -Wl,-rpath,/usr/local/lib

IMGUI_SRC = \
  imgui/imgui.cpp \
  imgui/imgui_draw.cpp \
  imgui/imgui_tables.cpp \
  imgui/imgui_widgets.cpp \
  imgui/imgui_impl_sdl3.cpp \
  imgui/imgui_impl_opengl3.cpp

APP_SRC = \
  gui/src/main_gui.cpp \
  gui/src/app.cpp \
  gui/src/window_connect.cpp \
  gui/src/window_chat.cpp \
  gui/src/window_server.cpp \
  gui/src/window_files.cpp \
  gui/src/window_settings.cpp \
  gui/src/security.cpp

SOURCES = $(IMGUI_SRC) $(APP_SRC)
OBJECTS = $(patsubst %.cpp,obj/%.o,$(notdir $(SOURCES)))

TARGET = SafeSocketGUI

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) -o $@ $^ $(LDFLAGS)

obj/%.o: imgui/%.cpp | obj
	$(CXX) $(CXXFLAGS) -c $< -o $@

obj/%.o: gui/src/%.cpp | obj
	$(CXX) $(CXXFLAGS) -c $< -o $@

obj:
	mkdir -p obj

clean:
	rm -rf obj $(TARGET)

.PHONY: all clean
