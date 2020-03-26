#pragma once

#include "platform_includes.h"

class Shader;

class Pointer{
public:
	Pointer();
	~Pointer();

public:
	void ApplyShader(Shader& shader);
	void Render(glm::mat4 projectionMatrix);

	void SetColorAndRadius(const float *color, float radius);
	void SetLength(float t, float extension = 0);
private:
	glm::vec3 color;
	float radius;
	float length, extension;
	GLuint _progId;
	GLuint _vaoId;
	GLuint _projId;
	GLuint _colorId;
	GLuint _verts;
};
