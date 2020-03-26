#pragma once

#include "platform_includes.h"
#include <ml_types.h>

class Controller {
public:
	Controller();
	~Controller();

	bool connected() const;
	void poll();

	float get_trigger() const;

	class ButtonHandler{
	public:
		virtual ~ButtonHandler(){}
		virtual void on_press() = 0;
	};
	void set_bumper_handler(ButtonHandler &handler);
	
	bool get_bumper() const;
	
	void get_pose(glm::vec3 &pos, glm::quat &rot) const;
	void get_pose(glm::mat4 &m) const;
	void get_ray(glm::vec3 &pos, glm::vec3 &dir) const;
	void get_touch(glm::vec3 &pos_and_force_delta) const;
public:
	MLHandle handle;
	bool is_connected;
	glm::vec3 pos;
	glm::quat rot;
	
	float trigger; // In range [0,1], 0 = not pressed, 1 = max pressed
	ButtonHandler *bumper_handler;
	bool bumper_down;
	MLVec3f touch_pos_and_force, prev_touch_pos_and_force;
};
