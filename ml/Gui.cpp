
#include "Gui.h"
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_opengl3.h"
#include "Controller.h"
#include "Shader.h"
#include <ml_input.h>
#include <vector>


const char APP_TAG[] = "vklBoards::Gui";
#include <ml_logging.h>

static constexpr int32_t kImguiQuadWidth = 512;
static constexpr int32_t kImguiQuadHeight = 512;
static constexpr float kCursorSpeed = kImguiQuadWidth * 20.f;
static constexpr float kPressThreshold = 0.1f;


Gui::Gui():
	imgui_color_texture_(0),
	imgui_framebuffer_(0),
	imgui_depth_renderbuffer_(0),
	state_(State::Hidden),
	pos_scale(1)
{}
Gui::~Gui() {}

void Gui::init(Controller *controller_){
	controller = controller_;
	
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui_ImplOpenGL3_Init("#version 410 core");

	// Setup back-end capabilities flags
	ImGuiIO &io = ImGui::GetIO();
	io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;

	// Keyboard mapping. ImGui will use those indices to peek into the io.KeysDown[] array.
	io.KeyMap[ImGuiKey_Tab] = MLKEYCODE_TAB;
	io.KeyMap[ImGuiKey_LeftArrow] = MLKEYCODE_DPAD_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = MLKEYCODE_DPAD_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow] = MLKEYCODE_DPAD_UP;
	io.KeyMap[ImGuiKey_DownArrow] = MLKEYCODE_DPAD_DOWN;
	io.KeyMap[ImGuiKey_PageUp] = MLKEYCODE_PAGE_UP;
	io.KeyMap[ImGuiKey_PageDown] = MLKEYCODE_PAGE_DOWN;
	io.KeyMap[ImGuiKey_Home] = MLKEYCODE_HOME;
	io.KeyMap[ImGuiKey_End] = MLKEYCODE_ENDCALL;
	io.KeyMap[ImGuiKey_Insert] = MLKEYCODE_INSERT;
	io.KeyMap[ImGuiKey_Delete] = MLKEYCODE_DEL;
	io.KeyMap[ImGuiKey_Backspace] = MLKEYCODE_BACK;
	io.KeyMap[ImGuiKey_Space] = MLKEYCODE_SPACE;
	io.KeyMap[ImGuiKey_Enter] = MLKEYCODE_ENTER;
	io.KeyMap[ImGuiKey_Escape] = MLKEYCODE_ESCAPE;
	io.KeyMap[ImGuiKey_A] = MLKEYCODE_A;
	io.KeyMap[ImGuiKey_C] = MLKEYCODE_C;
	io.KeyMap[ImGuiKey_V] = MLKEYCODE_V;
	io.KeyMap[ImGuiKey_X] = MLKEYCODE_X;
	io.KeyMap[ImGuiKey_Y] = MLKEYCODE_Y;
	io.KeyMap[ImGuiKey_Z] = MLKEYCODE_Z;


	glGenFramebuffers(1, &imgui_framebuffer_);
	glBindFramebuffer(GL_FRAMEBUFFER, imgui_framebuffer_);
	
	glGenTextures(1, &imgui_color_texture_);
	glBindTexture(GL_TEXTURE_2D, imgui_color_texture_);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kImguiQuadWidth, kImguiQuadHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, imgui_color_texture_, 0);

	glGenRenderbuffers(1, &imgui_depth_renderbuffer_);
	glBindRenderbuffer(GL_RENDERBUFFER, imgui_depth_renderbuffer_);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, kImguiQuadWidth, kImguiQuadHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, imgui_depth_renderbuffer_);

	GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		ML_LOG(Fatal, "Framebuffer is not complete!");
	}
	ImGui::StyleColorsDark();
	
	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(static_cast<float>(kImguiQuadWidth), static_cast<float>(kImguiQuadHeight)), ImGuiCond_FirstUseEver);
	
}

void Gui::cleanup() {
	glDeleteTextures(1, &imgui_color_texture_);
	glDeleteFramebuffers(1, &imgui_framebuffer_);
	glDeleteRenderbuffers(1, &imgui_depth_renderbuffer_);
	ImGui_ImplOpenGL3_Shutdown();
}

void Gui::ApplyShader(Shader& shader) {
	Shader g;
	g.load_shaders("shader/gui.vert", "shader/gui.frag");
	guiprog = g.get_program_id();




	_progId = shader.get_program_id();
	glUseProgram(_progId);

	_colorId = shader.get_uniform_loc("color");
	_projId = shader.get_uniform_loc("projFrom3D");
	GLuint location = shader.get_attrib_loc("coord3D");
	unifTex = shader.get_attrib_loc("tex2D");

	glGenVertexArrays(1, &_vaoId);
	glBindVertexArray(_vaoId);

	float sz = 0.25;
	GLfloat VertexData[20] = {
		-sz,  sz, 0, 0,1,
		 sz,  sz, 0, 1,1,
		-sz, -sz, 0, 0,0,
		 sz, -sz, 0, 1,0
	};

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(VertexData), VertexData, GL_STATIC_DRAW);
	glVertexAttribPointer(location, 3, GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), reinterpret_cast<void*>(0));
	glVertexAttribPointer(unifTex, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), reinterpret_cast<void*>(3*sizeof(float)));
	glEnableVertexAttribArray(location);
	glEnableVertexAttribArray(unifTex);

	glBindVertexArray(0);
	glUseProgram(0);

	//ML_LOG_TAG(Debug, APP_TAG, "Gui Uniform location (%d, %d), program %d", _projId, location, _progId);
}

void Gui::Render(glm::mat4 projectionMatrix) {
	if (state_ == State::Hidden) {
		return;
	}
	glUseProgram(_progId);
	
	glm::mat4 transform;
	glm::vec3 z = pos_normal;
	glm::vec3 y = glm::vec3(0, 1, 0);
	y -= glm::dot(z, y) * z;
	y = glm::normalize(y);
	glm::vec3 x = glm::cross(y, z);
	float s = pos_scale;
	transform[0][0] = s*x[0];
	transform[0][1] = s*x[1];
	transform[0][2] = s*x[2];
	transform[0][3] = 0;
	transform[1][0] = s*y[0];
	transform[1][1] = s*y[1];
	transform[1][2] = s*y[2];
	transform[1][3] = 0;
	transform[2][0] = s*z[0];
	transform[2][1] = s*z[1];
	transform[2][2] = s*z[2];
	transform[2][3] = 0;
	transform[3][0] = pos_location[0];
	transform[3][1] = pos_location[1];
	transform[3][2] = pos_location[2];
	transform[3][3] = 1;
	
	transform = projectionMatrix * transform;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, imgui_color_texture_);
	glBindVertexArray(_vaoId);
	glUniform4f(_colorId, 0, 0, 0, 0);
	glUniformMatrix4fv(_projId, 1, GL_FALSE, &transform[0][0]);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glUseProgram(0);
}

bool Gui::is_placed() const{
	return (state_ == State::Placed);
}
bool Gui::is_visible() const{
	return (state_ != State::Hidden);
}

void Gui::get_placement_pose(glm::vec3 &pos, glm::quat &rot) const{
	pos = placement_pos;
	rot = placement_rot;
}

void Gui::update_begin() {
	ImGuiIO &io = ImGui::GetIO();

	io.DisplaySize = ImVec2((float)kImguiQuadWidth, (float)kImguiQuadHeight);
	io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

	// Setup time step
	auto current_time = std::chrono::steady_clock::now();
	std::chrono::duration<float> delta_time = current_time - previous_time_;
	io.DeltaTime = delta_time.count();
	previous_time_ = current_time;
	io.MouseDrawCursor = true;
	
	// Handle input
	update_state();
	if (state_ != State::Hidden) {
		update_input();
	}
	
	// Start the ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	IM_ASSERT(io.Fonts->IsBuilt());
	ImGui::NewFrame();

	// Set default position and size for new windows
	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(static_cast<float>(kImguiQuadWidth), static_cast<float>(kImguiQuadHeight)), ImGuiCond_FirstUseEver);
}

void Gui::update_end() {
	ImGui::Render();

	// Off-screen render
	glBindFramebuffer(GL_FRAMEBUFFER, imgui_framebuffer_);

	glUseProgram(guiprog);
	glClearColor(0.f, 0.f, 0.f, 0.f);
	glClearDepthf(1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Gui::update_state() {
	if (NULL == controller) { return;  }
	bool toggle_state = controller->bumper_down;
	if (toggle_state && !prev_toggle_state_) {
		
		switch (state_) {
		case State::Hidden: state_ = State::Moving; break;
		case State::Moving: state_ = State::Placed; break;
		case State::Placed: state_ = State::Hidden; break;
		}
		/*
		switch (state_) {
		case State::Hidden: ML_LOG_TAG(Debug, APP_TAG, "GUI hidden"); break;
		case State::Moving: ML_LOG_TAG(Debug, APP_TAG, "GUI moving"); break;
		case State::Placed: ML_LOG_TAG(Debug, APP_TAG, "GUI placed"); break;
		}
		*/
	}
	prev_toggle_state_ = toggle_state;

	if(state_ == State::Moving){
		controller->get_pose(placement_pos, placement_rot);
		glm::mat3 rotmat = glm::toMat3(placement_rot);
		pos_location = placement_pos - 0.75f*rotmat[2];
		pos_normal = rotmat[2];
	}
}

void Gui::update_input() {
	// Update buttons
	ImGuiIO &io = ImGui::GetIO();

	bool prev_mouse_down[5] = {};
	std::memcpy(prev_mouse_down, io.MouseDown, 5);

	io.MouseDown[0] = controller->get_trigger() > kPressThreshold;

	for (size_t i = 0; i < 5; ++i) {
		if (io.MouseDown[i] && !prev_mouse_down[i]) {
			MLInputStartControllerFeedbackPatternVibe(controller->handle, 0, MLInputControllerFeedbackPatternVibe_Click, MLInputControllerFeedbackIntensity_High);
		}
	}
	
	glm::vec3 delta;
	controller->get_touch(delta);
	delta *= io.DeltaTime * kCursorSpeed;
	cursor_pos[0] = glm::clamp(cursor_pos[0] + delta[0], 0.f, (float)kImguiQuadWidth);
	cursor_pos[1] = glm::clamp(cursor_pos[1] - delta[1], 0.f, (float)kImguiQuadHeight);
	io.MousePos = ImVec2(cursor_pos[0], cursor_pos[1]);
}

