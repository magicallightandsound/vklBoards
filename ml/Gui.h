#pragma once

#include "platform_includes.h"
#include <chrono>
class Shader;
class Controller;

class Gui {
public:
	Gui();
	~Gui();

public:
	void ApplyShader(Shader& shader);
	void Render(glm::mat4 projectionMatrix);
	
	void init(Controller *controller);
	void cleanup();

	bool is_placed() const;
	bool is_visible() const;
	
	void update_begin(); // returns true when gui is first placed
	void update_end();
	void update_state();
	void update_input();

	virtual void BuildGUI(){}
	
	void get_placement_pose(glm::vec3 &pos, glm::quat &rot) const;
private:
	//std::shared_ptr<Texture> off_screen_texture_;
	//MLHandle input_handle_;
	//MLVec3f prev_touch_pos_and_force_;
	//glm::vec2 cursor_pos_;
	enum class State { Hidden, Moving, Placed } state_;
	bool prev_toggle_state_ = false;

	GLuint guiprog;
	GLuint _progId;
	GLuint _vaoId;
	GLuint _projId;
	GLuint _colorId;
	
	Controller *controller;
	GLuint imgui_color_texture_;
	GLuint imgui_framebuffer_;
	GLuint imgui_depth_renderbuffer_;
	GLuint off_screen_texture_;
	GLuint unifTex;

	std::chrono::steady_clock::time_point previous_time_;
	
	// Position of controller when the GUI was placed
	glm::vec3 placement_pos;
	glm::quat placement_rot;
	
	glm::vec3 pos_location;
	glm::vec3 pos_normal;
	float pos_scale;
	int cursor_pos[2];
};
