#include "Controller.h"
#include <ml_input.h>

const char APP_TAG[] = "vklBoards::Controller";
#include <ml_logging.h>

void Controller::poll(){
	MLInputControllerState input_states[MLInput_MaxControllers];
	MLResult result = MLInputGetControllerState(handle, input_states);
	if(MLResult_Ok != result){
		ML_LOG_TAG(Debug, APP_TAG, "MLInputGetControllerState returned %d", (int)result);
	}
	
	MLTransform controller_transform;
	for(int k = 0; k < MLInput_MaxControllers; ++k){
		const MLInputControllerState &state = input_states[k];
		if(!state.is_connected){ continue; }
		is_connected = true;
		pos.x = state.position.x;
		pos.y = state.position.y;
		pos.z = state.position.z;
		rot.x = state.orientation.x;
		rot.y = state.orientation.y;
		rot.z = state.orientation.z;
		rot.w = state.orientation.w;
		
		trigger = state.trigger_normalized;
		
		bool bumper_down_current = state.button_state[MLInputControllerButton_Bumper];
		if(NULL != bumper_handler && !bumper_down && bumper_down_current){
			bumper_handler->on_press();
		}
		bumper_down = bumper_down_current;
		prev_touch_pos_and_force = touch_pos_and_force;
		touch_pos_and_force = state.touch_pos_and_force[k];
		break;
	}
}

Controller::Controller():
	handle(0), is_connected(false),
	pos(0, 0, 0),
	rot(0, 0, 0, 1),
	bumper_handler(NULL),
	bumper_down(false)
{
	MLResult result = MLInputCreate(NULL, &handle);
	if(MLResult_Ok != result){
		ML_LOG_TAG(Debug, APP_TAG, "MLInputCreate returned %d", (int)result);
	}
}

Controller::~Controller() {
	MLInputDestroy(handle);
}

bool Controller::connected() const{
	return is_connected;
}

float Controller::get_trigger() const{
	return trigger;
}

bool Controller::get_bumper() const{
	return bumper_down;
}

void Controller::set_bumper_handler(ButtonHandler &handler){
	bumper_handler = &handler;
}

void Controller::get_pose(glm::vec3 &pos_, glm::quat &rot_) const{
	pos_ = pos;
	rot_ = rot;
}
void Controller::get_pose(glm::mat4 &m) const{
    m = glm::translate(glm::mat4(), pos) * glm::toMat4(rot);
}

void Controller::get_ray(glm::vec3 &org, glm::vec3 &dir) const{
	org = pos;
	dir[0] = -2.f * (rot.x * rot.z + rot.w * rot.y);
	dir[1] = -2.f * (rot.y * rot.z + rot.w * rot.x);
	dir[2] = 2.f * (rot.x * rot.x + rot.y * rot.y) - 1.f;
}

void Controller::get_touch(glm::vec3 &pos_and_force_delta) const{
	if(touch_pos_and_force.z > 0.f && prev_touch_pos_and_force.z > 0.f){
		pos_and_force_delta[0] = touch_pos_and_force.x - prev_touch_pos_and_force.x;
		pos_and_force_delta[1] = touch_pos_and_force.y - prev_touch_pos_and_force.y;
		pos_and_force_delta[2] = touch_pos_and_force.z - prev_touch_pos_and_force.z;
	}else{
		pos_and_force_delta = glm::vec3(0, 0, 0);
	}
}
