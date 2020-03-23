CFLAGS = -O2 -g -Wall
CXXFLAGS = -Icommon -O2 -std=c++11 -g -Wall


ifeq ($(OS),Windows_NT)
	# Windows mingw64 on MSYS2
	NETLIBS = -lPocoNet -lPocoFoundation -lwsock32
	GFXLIBS = -lglfw3 -Lclip/build -lclip
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		NETLIBS = -lPocoNet -lPocoFoundation
		GFXLIBS = -lglfw3 -Lclip/build -lclip
	endif
	ifeq ($(UNAME_S),Darwin)
		# Mac OS
		NETLIBS = -lPocoNet -lPocoFoundation
		GFXLIBS = -lglfw -Lclip/build -lclip -framework Cocoa
	endif
endif

all: board_server guiclient

COMMON_OBJS = \
	obj/BoardServer.o \
	obj/BoardClient.o \
	obj/BoardContent.o \
	obj/fastlz.o \
	obj/lodepng.o \
	obj/ImageCoder.o
GUI_OBJS = \
	obj/imgui_impl_glfw.o \
	obj/imgui_impl_opengl3.o \
	obj/imgui.o \
	obj/imgui_draw.o \
	obj/imgui_widgets.o \
	obj/gl3w.o

board_server: pc/test_server.cpp $(COMMON_OBJS)
	c++ $(CXXFLAGS) -o $@ $^ $(NETLIBS)

guiclient: obj/main.o obj/QrCode.o $(COMMON_OBJS) $(GUI_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(GFXLIBS) $(NETLIBS)

obj/main.o: pc/main.cpp
	$(CXX) -c $(CXXFLAGS) $< -I. -I./imgui -I./gl3w/include -o $@

obj/imgui_impl_glfw.o: imgui/imgui_impl_glfw.cpp
	$(CXX) -DIMGUI_IMPL_OPENGL_LOADER_GL3W -c $(CXXFLAGS) $< -I./imgui -I./gl3w/include -o $@
obj/imgui_impl_opengl3.o: imgui/imgui_impl_opengl3.cpp
	$(CXX) -DIMGUI_IMPL_OPENGL_LOADER_GL3W -c $(CXXFLAGS) $< -I./imgui -I./gl3w/include -o $@
obj/imgui.o: imgui/imgui.cpp
	$(CXX) -c $(CXXFLAGS) $< -I./imgui -o $@
obj/imgui_draw.o: imgui/imgui_draw.cpp
	$(CXX) -c $(CXXFLAGS) $< -I./imgui -o $@
obj/imgui_widgets.o: imgui/imgui_widgets.cpp
	$(CXX) -c $(CXXFLAGS) $< -I./imgui -o $@
obj/BoardClient.o: common/BoardClient.cpp common/BoardMessage.h common/BoardClient.h common/BoardServer.h
	$(CXX) -c $(CXXFLAGS) $< -o $@
obj/BoardServer.o: common/BoardServer.cpp common/BoardMessage.h common/BoardServer.h
	$(CXX) -c $(CXXFLAGS) $< -o $@
obj/BoardContent.o: common/BoardContent.cpp common/BoardContent.h
	$(CXX) -c $(CXXFLAGS) $< -o $@
obj/QrCode.o: pc/QrCode.cpp pc/QrCode.hpp
	$(CXX) -c $(CXXFLAGS) $< -o $@
obj/ImageCoder.o: common/ImageCoder.cpp common/ImageCoder.h
	$(CXX) -c $(CXXFLAGS) $< -o $@
obj/fastlz.o: common/fastlz.c common/fastlz.h
	$(CC) -c $(CFLAGS) $< -o $@

obj/lodepng.o: common/lodepng.cpp common/lodepng.h
	$(CXX) -c $(CXXFLAGS) $< -o $@

obj/gl3w.o: gl3w/src/gl3w.c
	$(CC) -c $(CFLAGS) $< -I./gl3w/include -o $@


clean:
	rm -f obj/*.o guiclient board_server *.exe
