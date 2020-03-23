CFLAGS = -O2 -g -Wall
CXXFLAGS = -O2 -std=c++11 -g -Wall

# Mac OS
#NETLIBS = -lPocoNet -lPocoFoundation
#GFXLIBS = -lglfw

# Windows mingw64 on MSYS2
NETLIBS = -lPocoNet -lPocoFoundation -lwsock32
GFXLIBS = -lglfw3 -Lclip/build -lclip

all: board_server guiclient

board_server: test_server.cpp BoardServer.o fastlz.o ImageCoder.o
	c++ $(CXXFLAGS) -o $@ $^ $(NETLIBS)

guiclient: main.o imgui_impl_glfw.o imgui_impl_opengl3.o imgui.o imgui_draw.o imgui_widgets.o gl3w.o BoardClient.o BoardServer.o BoardContent.o lodepng.o  fastlz.o QrCode.o ImageCoder.o
	$(CXX) $(CXXFLAGS) $^ -o $@ $(GFXLIBS) $(NETLIBS)

main.o: main.cpp
	$(CXX) -c $(CXXFLAGS) $< -I. -I./imgui -I./gl3w/include -o $@

imgui_impl_glfw.o: imgui/imgui_impl_glfw.cpp
	$(CXX) -DIMGUI_IMPL_OPENGL_LOADER_GL3W -c $(CXXFLAGS) $< -I./imgui -I./gl3w/include -o $@
imgui_impl_opengl3.o: imgui/imgui_impl_opengl3.cpp
	$(CXX) -DIMGUI_IMPL_OPENGL_LOADER_GL3W -c $(CXXFLAGS) $< -I./imgui -I./gl3w/include -o $@
imgui.o: imgui/imgui.cpp
	$(CXX) -c $(CXXFLAGS) $< -I./imgui -o $@
imgui_draw.o: imgui/imgui_draw.cpp
	$(CXX) -c $(CXXFLAGS) $< -I./imgui -o $@
imgui_widgets.o: imgui/imgui_widgets.cpp
	$(CXX) -c $(CXXFLAGS) $< -I./imgui -o $@
BoardClient.o: BoardClient.cpp BoardMessage.h BoardClient.h BoardServer.h
	$(CXX) -c $(CXXFLAGS) $< -o $@
BoardServer.o: BoardServer.cpp BoardMessage.h BoardServer.h
	$(CXX) -c $(CXXFLAGS) $< -o $@
BoardContent.o: BoardContent.cpp BoardContent.h
	$(CXX) -c $(CXXFLAGS) $< -o $@
QrCode.o: QrCode.cpp QrCode.hpp
	$(CXX) -c $(CXXFLAGS) $< -o $@
ImageCoder.o: ImageCoder.cpp ImageCoder.h
	$(CXX) -c $(CXXFLAGS) $< -o $@
fastlz.o: fastlz.c fastlz.h
	$(CC) -c $(CFLAGS) $< -o $@

Clipboard.o: Clipboard.cpp Clipboard.h
	$(CXX) -c $(CXXFLAGS) $< -o $@
lodepng.o: lodepng.cpp lodepng.h
	$(CXX) -c $(CXXFLAGS) $< -o $@

gl3w.o: gl3w/src/gl3w.c
	$(CC) -c $(CFLAGS) $< -I./gl3w/include -o $@


clean:
	rm -f *.o *.exe