#include "Billboard.h"
#include "Shader.h"
#include "lodepng.h"

#include <iostream>

#define GLM_FORCE_RADIANS
#include "../glm/gtc/matrix_transform.hpp"

const char APP_TAG[] = "vklBoards::Billboard";
#include <ml_logging.h>

Billboard::Billboard(){
	_position = glm::vec3(0, 0, -0.7f);
	_scale = glm::vec3(0.25);
	
	image = NULL;
}

Billboard::~Billboard() {
	free(image);
}

void Billboard::load(const char *texfile){
	if(NULL != image){ free(image); }
	unsigned error = lodepng_decode32_file(&image, &width, &height, texfile);
	if(error){
		ML_LOG_TAG(Debug, APP_TAG, "lodepng error %u: %s", error, lodepng_error_text(error));
	}
}

void Billboard::ApplyShader(Shader& shader) {
	_progId = shader.get_program_id();
	glUseProgram(_progId);

	_projId = shader.get_uniform_loc("projFrom3D");
	GLuint location = shader.get_attrib_loc("coord3D");
	GLuint location2 = shader.get_attrib_loc("tex2D");

	glGenTextures(1, &_texId);
	glBindTexture(GL_TEXTURE_2D, _texId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenVertexArrays(1, &_vaoId);
	glBindVertexArray(_vaoId);

	GLfloat w = (GLfloat)width / (GLfloat)height;
	GLfloat VertexData[20] = {
		-w, -1, 0, 0,1,
		 w, -1, 0, 1,1,
		-w,  1, 0, 0,0,
		 w,  1, 0, 1,0
	};

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(VertexData), VertexData, GL_STATIC_DRAW);
	glVertexAttribPointer(location, 3, GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), reinterpret_cast<void*>(0));
	glVertexAttribPointer(location2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), reinterpret_cast<void*>(3*sizeof(float)));
	glEnableVertexAttribArray(location);
	glEnableVertexAttribArray(location2);

	glBindVertexArray(0);
	glUseProgram(0);

	ML_LOG_TAG(Debug, APP_TAG, "Billboard width/height = %u, %u", width, height);
	ML_LOG_TAG(Debug, APP_TAG, "Billboard Uniform location (%d, %d), program %d", _projId, location, _progId);
}

void Billboard::Render(glm::mat4 projectionMatrix) {
	glUseProgram(_progId);

	glm::mat4 translate = glm::translate(glm::mat4(1.0f), _position);
	glm::mat4 scale = glm::scale(_scale);
	glm::mat4 transform = projectionMatrix * translate * scale;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, _texId);
	glBindVertexArray(_vaoId);
	glUniformMatrix4fv(_projId, 1, GL_FALSE, &transform[0][0]);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glUseProgram(0);
}
