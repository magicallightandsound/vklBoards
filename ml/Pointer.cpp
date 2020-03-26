#include "Pointer.h"
#include "Shader.h"
#include "lodepng.h"

#include <iostream>

#define GLM_FORCE_RADIANS
#include "../glm/gtc/matrix_transform.hpp"

const char APP_TAG[] = "vklBoards::Pointer";
#include <ml_logging.h>

Pointer::Pointer(){
}
Pointer::~Pointer(){
}

static void MakeCylinder(GLfloat *vertices, int numSteps) {
	float step = 2 * M_PI / static_cast<float>(numSteps);
	float a = 0;
	float r = 1;
	int idx = 0;

	for (int i = 0; i < numSteps + 1; i++) {
		float x = cosf(a);
		float z = sinf(a);

		vertices[idx++] = x * r;
		vertices[idx++] = z * r;
		vertices[idx++] = -1.0;
		vertices[idx++] = x * r;
		vertices[idx++] = z * r;
		vertices[idx++] = 0.0;
		
		a += step;
	}
}

void Pointer::ApplyShader(Shader& shader) {
	_progId = shader.get_program_id();
	glUseProgram(_progId);

	_colorId = shader.get_uniform_loc("color");
	_projId = shader.get_uniform_loc("projFrom3D");
	GLuint location = shader.get_attrib_loc("coord3D");

	glGenVertexArrays(1, &_vaoId);
	glBindVertexArray(_vaoId);

	GLfloat cylinderVertexData[(16+1)*2 * 3];
	MakeCylinder(cylinderVertexData, 16);

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glEnableVertexAttribArray(location);
	glVertexAttribPointer(location, 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(0));
	glBufferData(GL_ARRAY_BUFFER, sizeof(cylinderVertexData), cylinderVertexData, GL_STATIC_DRAW);

	glBindVertexArray(0);
	glUseProgram(0);

	ML_LOG_TAG(Debug, APP_TAG, "Uniform location (%d, %d, %d), program %d", _colorId, _projId, location, _progId);
}

void Pointer::SetColorAndRadius(const float *color_, float radius_) {
	color[0] = color_[0];
	color[1] = color_[1];
	color[2] = color_[2];
	radius = radius_;
}

void Pointer::SetLength(float t, float e){
	length = t;
	extension = e;
}

void Pointer::Render(glm::mat4 projectionMatrix) {
	glUseProgram(_progId);

	glm::mat4 scale = glm::scale(glm::vec3(radius, radius, length));
	glm::mat4 transform = projectionMatrix * scale;

	glBindVertexArray(_vaoId);

	glUniformMatrix4fv(_projId, 1, GL_FALSE, &transform[0][0]);
	glUniform4f(_colorId, color[0], color[1], color[2], 1.f);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, (16+1)*2);

	// Draw extension
	glm::mat4 xlat = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, -length-0.001)); // Add 1mm gap
	scale = glm::scale(glm::vec3(radius, radius, extension));
	transform = projectionMatrix * xlat * scale;
	glUniformMatrix4fv(_projId, 1, GL_FALSE, &transform[0][0]);
	glUniform4f(_colorId, color[0], color[1], color[2], 0.2f);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, (16+1)*2);

	glBindVertexArray(0);

	glUseProgram(0);
}
