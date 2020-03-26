#pragma once

#include "platform_includes.h"

class Shader;

class Billboard {
public:
	Billboard();
	~Billboard();
	void load(const char *texfile);
public:
	void ApplyShader(Shader& shader);
	void Render(glm::mat4 projectionMatrix);
private:
	GLuint _progId;
	GLuint _vaoId;
	GLuint _projId;
	GLuint _texId;

	unsigned int width, height;
	unsigned char *image;

	glm::vec3 _position;
	glm::vec3 _scale;
};
