#pragma once

#include "platform_includes.h"

#include <string>

class Shader {
public:
	Shader() {}
	~Shader() {}

	GLuint get_program_id() { return progID; }
	GLuint get_attrib_loc(const std::string &name);
	GLuint get_uniform_loc(const std::string &name);
	
	void load_shaders(const char *vertex, const char *fragment);
	
private:
	GLuint progID;
};
