CXXFLAGS = -O2 -g -Wall

NETLIBS = -lPocoNet -lPocoFoundation -lwsock32
GFXLIBS = -lglfw3

all: board_server guiclient

board_server: test_server.cpp BoardServer.o
	c++ -o $@ test_server.cpp BoardServer.o $(NETLIBS)

guiclient: main.o imgui_impl_glfw.o imgui_impl_opengl3.o imgui.o imgui_draw.o imgui_widgets.o gl3w.o BoardClient.o BoardServer.o BoardContent.o lodepng.o QrCode.o
	$(CXX) $^ -o $@ $(GFXLIBS) $(NETLIBS)

main.o: main.cpp
	$(CXX) -c $(CXXFLAGS) $< -I. -I./imgui -I./gl3w/include -o $@

imgui_impl_glfw.o: imgui/imgui_impl_glfw.cpp
	$(CXX) -c $(CXXFLAGS) $< -I./imgui -I./gl3w/include -o $@
imgui_impl_opengl3.o: imgui/imgui_impl_opengl3.cpp
	$(CXX) -c $(CXXFLAGS) $< -I./imgui -I./gl3w/include -o $@
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

Clipboard.o: Clipboard.cpp Clipboard.h
	$(CXX) -c $(CXXFLAGS) $< -o $@
lodepng.o: lodepng.cpp lodepng.h
	$(CXX) -c $(CXXFLAGS) $< -o $@

gl3w.o: gl3w/src/gl3w.c
	$(CC) -c $(CFLAGS) $< -I./gl3w/include -o $@


clean:
	rm -f *.o *.exe