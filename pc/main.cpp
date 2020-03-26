// dear imgui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// If you are new to dear imgui, see examples/README.txt and documentation at the top of imgui.cpp.
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
#define IMGUI_IMPL_OPENGL_LOADER_GL3W
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "BoardClient.h"
#include "BoardContent.h"
#include "ImageCoder.h"
#include "lodepng.h"
#include "QrCode.hpp"
#include <cstdio>
#include <sstream>

// Clipboard support
#include "clip/clip.h"

#define GLM_ENABLE_EXPERIMENTAL 1
#include "../glm/gtx/quaternion.hpp"

#include "../glm/glm.hpp"
#include "../glm/ext.hpp"
#include "../glm/gtx/vector_angle.hpp"
#include "../glm/gtx/transform.hpp" 
#include "../glm/gtc/matrix_transform.hpp"

// About Desktop OpenGL function loaders:
//  Modern desktop OpenGL doesn't have a standard portable header file to load OpenGL function pointers.
//  Helper libraries are often used for this purpose! Here we are supporting a few common ones (gl3w, glew, glad).
//  You may use another loader/header of your choice (glext, glLoadGen, etc.), or chose to manually implement your own.
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
#include <GL/gl3w.h>    // Initialize with gl3wInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
#include <GL/glew.h>    // Initialize with glewInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
#include <glad/glad.h>  // Initialize with gladLoadGL()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING)
#define GLFW_INCLUDE_NONE         // GLFW including OpenGL headers causes ambiguity or multiple definition errors.
#include <glbinding/glbinding.h>  // Initialize with glbinding::initialize()
#include <glbinding/gl/gl.h>
using namespace gl;
#else
#include IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#endif

// Include glfw3.h after our OpenGL definitions
#include <GLFW/glfw3.h>

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

static unsigned rounduppow2(unsigned v){ // compute the next highest power of 2 of 32-bit v
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}

// We subclass both BoardClient and BoardContent, which is a bit confusing.
// BoardClient is the client interface to the server which manages data interchange for all boards residing on the server.
// BoardContent is the management interface for a single locally cached board.
// Our intent here is to have the BoardContent be a "view" into a single board obtained from the BoardClient.
struct MyBoard : public BoardClient, public BoardContent{
	std::vector<std::string> boards;
	std::vector<std::string> users;
	int iboard;

	GLuint texID, qrID;
	
	qrcodegen::QrCode qr;
	unsigned qrsize, qrtexsize;

	MyBoard():
		iboard(-1),
		texID(0),
		qr(qrcodegen::QrCode::encodeText("none", qrcodegen::QrCode::Ecc::HIGH))
	{
	}
	int connect(const std::string &server_uri, const std::string &name){
		int ret = BoardClient::connect(server_uri, name);
		if(0 != ret){
			printf(" connect returned %d\n", ret);
			return ret;
		}
		get_boards(boards);
		get_users(users);
		if(boards.size() > 0){
			iboard = 0;
			BoardClient::get_size(iboard, width, height);
			image.resize(3*width*height);
			get_contents(iboard, &image[0]);
			updatetex(&image[0], width, 0, 0, width, height);
			draw_gui(&image[3*(width - 64)], width, 64, height);
		}
		
		//std::string uri1 = this->connection.socket.address().toString();
		std::string uri2 = this->connection.socket.peerAddress().toString();
		//printf("uri1: %s\n", uri1.c_str());
		//printf("uri2: %s\n", uri2.c_str());
		qr = qrcodegen::QrCode::encodeText(uri2.c_str(), qrcodegen::QrCode::Ecc::HIGH);
		qrsize = qr.getSize();
		qrtexsize = rounduppow2(qrsize);
		
		return ret;
	}
	
	void gentex(){
		// Generate texture for the board content
		glGenTextures(1, &texID);
		glBindTexture(GL_TEXTURE_2D, texID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, &image[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
		
		// Generate QR code
		std::vector<unsigned char> qrbits(3*qrtexsize*qrtexsize);
		int off = (qrtexsize - qrsize)/2;
		for(int j = 0; j < qrtexsize; ++j){
			for(int i = 0; i < qrtexsize; ++i){
				unsigned char val = 0xff;
				if(qr.getModule(i-off, j-off)){
					val = 0x00;
				}
				qrbits[3*(i+j*qrtexsize)+0] = val;
				qrbits[3*(i+j*qrtexsize)+1] = val;
				qrbits[3*(i+j*qrtexsize)+2] = val;
			}
		}
		// Generate QR code texture
		glGenTextures(1, &qrID);
		glBindTexture(GL_TEXTURE_2D, qrID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, qrtexsize, qrtexsize, 0, GL_RGB, GL_UNSIGNED_BYTE, &qrbits[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	
	void on_image_update(Region *touched = NULL){
		if(NULL == touched){
			send_update(iboard, &image[0], width, 0, 0, width, height);
			updatetex(&image[0], width, 0, 0, width, height);
		}else{
			send_update(iboard, &image[0], width, touched->x, touched->y, touched->w, touched->h);
			updatetex(&image[0], width, touched->x, touched->y, touched->w, touched->h);
		}
	}
	void on_user_connected(const std::string &name){
		users.push_back(name);
	}
	void on_user_disconnected(const std::string &name){
		std::vector<std::string>::iterator it = std::find(users.begin(), users.end(), name);
		if(it != users.end()){
			users.erase(it);
		}
	}
	
	void updatetex(const unsigned char *data, unsigned stride, unsigned x, unsigned y, unsigned w, unsigned h){
		if(0 == texID){ return; }
		if(x >= width || y >= height){ return; }
		if(x+w >  width){ w =  width-x; }
		if(y+h > height){ h = height-y; }
		glBindTexture(GL_TEXTURE_2D, texID);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, width);
		glTexSubImage2D(GL_TEXTURE_2D, 0,
			x, y, w, h,
			GL_RGB, GL_UNSIGNED_BYTE,
			&data[3*(x+y*stride)]
		);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		// If we don't have GL_UNPACK_ROW_LENGTH, we must send entire width
		//glTexSubImage2D(GL_TEXTURE_2D, 0, 0, y, width, h, GL_RGB, GL_UNSIGNED_BYTE, &image[3 * (x + y*stride)]);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	
	// This is called when we have network data
	void on_update(board_index iboard_, int method, const unsigned char *buffer, unsigned buflen, unsigned x, unsigned y, unsigned w, unsigned h){
		if(iboard != iboard_){ return; }
		ImageCoder::decode(method,
			buffer, buflen,
			&image[3*(x+y*width)], width, w, h
		);
		updatetex(&image[0], width, x, y, w, h);
	}
	
	void switch_board(int i){
		if(0 <= i && i < boards.size()){
			iboard = i;
			BoardClient::get_size(iboard, width, height);
			image.resize(3*width*height);
			get_contents(iboard, &image[0]);
			draw_gui(&image[3*(width - 64)], width, 64, height);
			updatetex(&image[0], width, 0, 0, width, height);
		}
	}
	void add_board(const std::string &title){
		new_board(title, 2048, 1024);
		get_boards(boards);
	}
};

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos){
	const ImGuiIO& io = ImGui::GetIO();
	if(io.WantCaptureMouse){ return; }
	MyBoard &board = *(MyBoard*)glfwGetWindowUserPointer(window);

	int display_w, display_h;
	glfwGetWindowSize(window, &display_w, &display_h);

	unsigned width, height;
	board.BoardContent::get_size(width, height);
	float tx = (float)xpos/(float)display_w - 0.5;
	float ty = (float)ypos/(float)display_h - 0.5;
	float x, y;
	if(display_w * height > display_h * width){ // wide
		x = 0.5f*width + (float)height/display_h*display_w*tx;
		y = height*(0.5f+ty);
	}else{
		x = width*(0.5f+tx);
		y = 0.5f*height + (float)width/display_w*display_h*ty;
	}
	
	int px = (int)(x+0.5);
	int py = (int)(y+0.5);
	//board.pen_move(px, py);
	
	board.gui_input(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS, px, py);
}
static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods){
	const ImGuiIO& io = ImGui::GetIO();
	if(io.WantCaptureMouse){ return; }
	MyBoard &board = *(MyBoard*)glfwGetWindowUserPointer(window);
	
	
	int display_w, display_h;
	glfwGetWindowSize(window, &display_w, &display_h);
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	unsigned width, height;
	board.BoardContent::get_size(width, height);
	float tx = (float)xpos/(float)display_w - 0.5;
	float ty = (float)ypos/(float)display_h - 0.5;
	float x, y;
	if(display_w * height > display_h * width){ // wide
		x = 0.5f*width + (float)height/display_h*display_w*tx;
		y = height*(0.5f+ty);
	}else{
		x = width*(0.5f+tx);
		y = 0.5f*height + (float)width/display_w*display_h*ty;
	}
	
	int px = (int)(x+0.5);
	int py = (int)(y+0.5);
	board.gui_input(true, px, py);
}
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
	MyBoard &board = *(MyBoard*)glfwGetWindowUserPointer(window);
	if(GLFW_PRESS == action){
		if(key == GLFW_KEY_ESCAPE){
			glfwSetWindowShouldClose(window, GLFW_TRUE);
		}else if(key == GLFW_KEY_V){
			if(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL)){
				clip::image clipimg;
				if(clip::get_image(clipimg)){ // got image from clipboard
					unsigned w = clipimg.spec().width;
					unsigned h = clipimg.spec().height;
					std::vector<unsigned char> img(3*w*h);
					if(clipimg.spec().bits_per_pixel == 32){
						unsigned rawstride = clipimg.spec().bytes_per_row;
						unsigned char *dstrow = &img[0];
						unsigned char *srcrow = (unsigned char *)clipimg.data();
						for(unsigned j = 0; j < h; ++j){
							unsigned char *dstptr = dstrow;
							unsigned char *srcptr = srcrow;
							for(unsigned int i = 0; i < w; ++i){
								dstptr[0] = srcptr[0];
								dstptr[1] = srcptr[1];
								dstptr[2] = srcptr[2];
								dstptr += 3;
								srcptr += 4;
							}
							srcrow += rawstride;
							dstrow += 3*w;
						}
						board.paste_image(
							BoardContent::PASTE_LOC_CENTERED, BoardContent::PASTE_FORMAT_RGBA,
							&img[0], 3*w, 3,
							w, h
						);
					}
				}
			}
		}
	}
}
static void drop_callback(GLFWwindow* window, int count, const char** paths){
	MyBoard &board = *(MyBoard*)glfwGetWindowUserPointer(window);
	int i;
	for (i = 0;  i < count;  i++){
		unsigned error;
		unsigned char* image = 0;
		unsigned width, height;
//printf("file: %s\n", paths[i]);
		error = lodepng_decode32_file(&image, &width, &height, paths[i]);
		if(!error){
			board.paste_image(
				BoardContent::PASTE_LOC_CENTERED, BoardContent::PASTE_FORMAT_RGBA,
				image, 4*width, 4,
				width, height
			);
			free(image);
		}
	}
}


static const char *shader_vert = 
"#version 330 core\n"
"\n"
"layout(location = 0) in vec3 coord3D;\n"
"layout(location = 1) in vec2 tex2D;\n"
"uniform mat4 projFrom3D;\n"
"out vec2 texcoord;\n"
"\n"
"void main(){\n"
"	gl_Position =  projFrom3D * vec4(coord3D, 1);\n"
"	texcoord = tex2D;\n"
"}\n";
static const char *shader_frag = 
"#version 330 core\n"
"\n"
"in vec2 texcoord;\n"
"out vec4 fragColor;\n"
"uniform sampler2D tex;\n"
"\n"
"void main(void) {\n"
"	fragColor = texture(tex, texcoord);\n"
"}\n";

int main(int argc, char *argv[]){
	std::string uri;
	std::string myname = "<anonymous>";
	if(argc < 2){
		fprintf(stderr, "Provide server URI\n");
		return 1;
	}
	uri = argv[1];
	if(argc > 2){
		myname = argv[2];
	}
	MyBoard board;
	if(0 != board.connect(uri, myname)){
		fprintf(stderr, "Could not connect to server\n");
		return 1;
	}
	
	// Setup window
	glfwSetErrorCallback(glfw_error_callback);
	
	if (!glfwInit())
		return 1;

	// Decide GL+GLSL versions
#if __APPLE__
	// GL 3.2 + GLSL 150
	const char* glsl_version = "#version 150";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
	// GL 3.0 + GLSL 130
	const char* glsl_version = "#version 130";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

	// Create window with graphics context
	GLFWwindow* window = glfwCreateWindow(1280, 720, "Dear ImGui GLFW+OpenGL3 example", NULL, NULL);
	if (window == NULL)
		return 1;
	
	glfwSetWindowUserPointer(window, &board);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetKeyCallback(window, key_callback);
	glfwSetDropCallback(window, drop_callback);
	
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Enable vsync

	// Initialize OpenGL loader
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
	bool err = gl3wInit() != 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
	bool err = glewInit() != GLEW_OK;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
	bool err = gladLoadGL() == 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING)
	bool err = false;
	glbinding::initialize([](const char* name) { return (glbinding::ProcAddress)glfwGetProcAddress(name); });
#else
	bool err = false; // If you use IMGUI_IMPL_OPENGL_LOADER_CUSTOM, your loader is likely to requires some form of initialization.
#endif
	if (err)
	{
		fprintf(stderr, "Failed to initialize OpenGL loader!\n");
		return 1;
	}

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer bindings
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	// Our state
	GLuint pvID, pfID, progID, vaoID, vbo;
	GLint result, InfoLogLength;
	
	pvID = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(pvID, 1, &shader_vert, NULL);
	glCompileShader(pvID);
	glGetShaderiv(pvID, GL_COMPILE_STATUS, &result);
	glGetShaderiv(pvID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> ShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(pvID, InfoLogLength, NULL, &ShaderErrorMessage[0]);
		printf("Error compile vertex shader: '%s'", &ShaderErrorMessage[0]);
		return 1;
	}
	
	pfID = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(pfID, 1, &shader_frag, NULL);
	glCompileShader(pfID);
	glGetShaderiv(pfID, GL_COMPILE_STATUS, &result);
	glGetShaderiv(pfID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> ShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(pfID, InfoLogLength, NULL, &ShaderErrorMessage[0]);
		printf("Error compile vertex shader: '%s'", &ShaderErrorMessage[0]);
		return 1;
	}
	
	progID = glCreateProgram();
	glAttachShader(progID, pvID);
	glAttachShader(progID, pfID);
	glLinkProgram(progID);
	glGetProgramiv(progID, GL_LINK_STATUS, &result);
	glGetProgramiv(progID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> ErrorMessage(InfoLogLength + 1);
		glGetProgramInfoLog(progID, InfoLogLength, NULL, &ErrorMessage[0]);
		printf("Error link shader: '%s'", &ErrorMessage[0]);
		return 2;
	}
	glDeleteShader(pvID);
	glDeleteShader(pfID);
	
	glUseProgram(progID);
	GLuint unifProj = glGetUniformLocation(progID, "projFrom3D");
	GLuint unifVert = glGetAttribLocation(progID, "coord3D");
	GLuint unifTex  = glGetAttribLocation(progID, "tex2D");
	
	unsigned int width, height;
	board.BoardContent::get_size(width, height);
	//printf("board size %u %u\n", width, height);
	board.gentex();

	glGenVertexArrays(1, &vaoID);
	glBindVertexArray(vaoID);

	GLfloat w = 0.5f*(float)width/(float)height;
	GLfloat VertexData[20] = {
		-w, -0.5, 0, 0,1,
		 w, -0.5, 0, 1,1,
		-w,  0.5, 0, 0,0,
		 w,  0.5, 0, 1,0
	};
	
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(VertexData), VertexData, GL_STATIC_DRAW);
	glVertexAttribPointer(unifVert, 3, GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), reinterpret_cast<void*>(0));
	glVertexAttribPointer(unifTex, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), reinterpret_cast<void*>(3*sizeof(float)));
	glEnableVertexAttribArray(unifVert);
	glEnableVertexAttribArray(unifTex);

	glBindVertexArray(0);
	glUseProgram(0);

	// Main loop
	while (!glfwWindowShouldClose(window))
	{
		board.poll();
		// Poll and handle events (inputs, window resize, etc.)
		// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
		// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
		// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
		// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
		glfwPollEvents();

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		{
			std::string window_title("[not connected]");
			if(board.is_connected()){
				board.get_server(window_title);
			}
			ImGui::Begin(window_title.c_str()); // create new window

			if(ImGui::ListBoxHeader("Boards")){
				for(int i = 0; i < board.boards.size(); ++i){
					if(ImGui::Selectable(board.boards[i].c_str(), i == board.iboard)){
						if(i != board.iboard){
							board.switch_board(i);
						}
					}
				}
				ImGui::ListBoxFooter();
			}
			if(ImGui::Button("New board")){
				std::stringstream title;
				title << "Board " << board.boards.size();
				board.add_board(title.str());
			}
			if(ImGui::Button("Export board")){
				char filename[32];
				time_t rawtime;
				struct tm *timeinfo;
				time(&rawtime);
				timeinfo = localtime(&rawtime);
				strftime(filename, 32, "board-%Y-%m-%d-%H-%M-%S.png", timeinfo);
				lodepng_encode24_file(filename, &board.image[0], width, height);
			}
			if(ImGui::Button("Exit")){
				break;
			}
			
			if(ImGui::CollapsingHeader("QR code")){
				ImGui::Image((void*)(intptr_t)board.qrID, ImVec2(1024, 1024));
			}
			
			
			if(ImGui::ListBoxHeader("Users")){
				for(int i = 0; i < board.users.size(); ++i){
					if(ImGui::Selectable(board.users[i].c_str(), false)){
						// do nothing
					}
				}
				ImGui::ListBoxFooter();
			}
			
			ImGui::End();
		}
		ImGui::Render();
		
		// Rendering
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(0,0,0,0);
		glClear(GL_COLOR_BUFFER_BIT);
		
		{
			glm::mat4 x;
			unsigned width, height;
			board.BoardContent::get_size(width, height);
			if(display_w * height > display_h * width){
				float r = 0.5f * (float)display_w / (float)display_h;
				x = glm::ortho(-r, r, -0.5f, 0.5f);
			}else{
				float r = (float)display_h / (float)display_w;
				x = glm::ortho(-1.f, 1.f, -r, r);
			}
			glUseProgram(progID);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, board.texID);
			glBindVertexArray(vaoID);
			glUniformMatrix4fv(unifProj, 1, GL_FALSE, &x[0][0]);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			glBindVertexArray(0);
			glBindTexture(GL_TEXTURE_2D, 0);
			glUseProgram(0);
		}
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
