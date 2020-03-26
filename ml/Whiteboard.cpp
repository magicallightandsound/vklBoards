#include "Whiteboard.h"
#include "BoardClient.h"
#include "Shader.h"
#include "lodepng.h"

#include <iostream>

#define GLM_FORCE_RADIANS
#include "../glm/gtc/matrix_transform.hpp"

const char APP_TAG[] = "vklBoards::Whiteboard";
#include <ml_logging.h>

Whiteboard::Whiteboard(BoardClient *client_, bool is_remote_, int board_index, const std::string &name_):
	BoardContent(),
	client(client_),
	name(name_),
	is_remote(is_remote_),
	iboard(board_index),
	has_placement(false),
	visible(false)
{
	position.location = glm::vec3(0,0,-2);
	position.normal = glm::vec3(0,0,1);
	position.scale = 0.75f;
	UpdateTransform();

	// Set up GUI
	draw_gui(&image[3*(width - 64)], width, 64, height);
}

Whiteboard::~Whiteboard(){
}

void Whiteboard::ApplyShader(Shader& shader) {
	_progId = shader.get_program_id();
	glUseProgram(_progId);

	_projId = shader.get_uniform_loc("projFrom3D");
	GLuint location = shader.get_attrib_loc("coord3D");
	GLuint location2 = shader.get_attrib_loc("tex2D");

	glGenTextures(1, &_texId);
	glBindTexture(GL_TEXTURE_2D, _texId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, &image[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenVertexArrays(1, &_vaoId);
	glBindVertexArray(_vaoId);

	GLfloat w = 0.5*width_height;
	GLfloat VertexData[20] = {
		-w, -0.5, 0, 0,1,
		 w, -0.5, 0, 1,1,
		-w,  0.5, 0, 0,0,
		 w,  0.5, 0, 1,0
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

	//ML_LOG_TAG(Debug, APP_TAG, "Whiteboard width/height = %u, %u", width, height);
	//ML_LOG_TAG(Debug, APP_TAG, "Whiteboard Uniform location (%d, %d), program %d", _projId, location, _progId);
}

void Whiteboard::Render(glm::mat4 projectionMatrix) {
	if(!visible){ return; }
	
	glUseProgram(_progId);

	glm::mat4 x = projectionMatrix * transform;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, _texId);
	glBindVertexArray(_vaoId);
	glUniformMatrix4fv(_projId, 1, GL_FALSE, &x[0][0]);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glUseProgram(0);
}



int Whiteboard::RayIntersection(const glm::vec3 &o_, const glm::vec3 &d_, glm::vec2 &p, float &t) const{
	if(!visible){ return 0; }
	// We have a plane at z = 0, transformed by 'transform'.
	// We thus need to apply the inverse transform to o and d
	glm::vec4 o = transform_inverse * glm::vec4(o_, 1);
	glm::vec4 d = transform_inverse * glm::vec4(d_, 0);
	// Now check for intersection against z = 0 plane
	t = -o[2] / d[2];
	p[0] = o[0] + t * d[0];
	p[1] = o[1] + t * d[1];

	float w = 0.5f*width_height;
	if (-w <= p[0] && p[0] <= w && -0.5 <= p[1] && p[1] <= 0.5) {
		return 1;
	}
	return 0;
}

void Whiteboard::set_visibility(bool state){ visible = state; }
bool Whiteboard::get_visibility() const{ return visible; }
const std::string &Whiteboard::get_name() const{ return name; }

void Whiteboard::ReceiveInput(const glm::vec3 &pos, const glm::quat &rot, bool pressed, const glm::vec2 &cursor_pos) {
	if(!visible){ return; }
	if (moving){
		if (pressed) {
			// Update current transform
			glm::mat3 rotmat = glm::toMat3(rot);
			position.location = pos - 1.f*rotmat[2];
			position.normal = rotmat[2];
			UpdateTransform();
		}else{
			moving = false;
		}
		return;
	}
	int x, y;
	coord2px(cursor_pos, x, y);
	gui_input(pressed, x, y);
}

void Whiteboard::coord2px(const glm::vec2 &p, int &x, int &y) const {
	float w = width_height;
	x = (int)((p[0] / w + 0.5f)*width + 0.5f);
	y = (int)((0.5f - p[1])*height + 0.5f);

}
void Whiteboard::px2coord(int x, int y, glm::vec2 &p) const {
	float w = width_height;
	p[0] = ((float)x / width - 0.5f) * w;
	p[1] = 0.5f - (float)y / height;
}

void Whiteboard::UpdateTexture(const unsigned char *data, unsigned stride, unsigned x, unsigned y, unsigned w, unsigned h){
	if(0 == _texId){ return; }
	if(x >= width || y >= height){ return; }
	if(x+w >  width){ w =  width-x; }
	if(y+h > height){ h = height-y; }
	glBindTexture(GL_TEXTURE_2D, _texId);
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

void Whiteboard::on_image_update(Region *touched){
	if(NULL == touched){
		if(NULL != client){
			client->send_update(iboard, &image[0], width, 0, 0, width, height);
		}
		UpdateTexture(&image[0], width, 0, 0, width, height);
	}else{
		if(NULL != client){
			client->send_update(iboard, &image[0], width, touched->x, touched->y, touched->w, touched->h);
		}
		UpdateTexture(&image[0], width, touched->x, touched->y, touched->w, touched->h);
	}
}


void Whiteboard::set_position(const glm::vec3 &pos, const glm::quat &rot){
	// Update current transform
	glm::mat3 rotmat = glm::toMat3(rot);
	position.location = pos - 1.f*rotmat[2];
	position.normal = rotmat[2];
	UpdateTransform();
	
	has_placement = true;
}

void Whiteboard::UpdateTransform() {
	glm::vec3 z = position.normal;
	glm::vec3 y = glm::vec3(0, 1, 0);
	y -= glm::dot(z, y) * z;
	y = glm::normalize(y);
	glm::vec3 x = glm::cross(y, z);
	float s = position.scale;
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
	transform[3][0] = position.location[0];
	transform[3][1] = position.location[1];
	transform[3][2] = position.location[2];
	transform[3][3] = 1;

	s = 1.f / position.scale;
	transform_inverse[0][0] = s * x[0];
	transform_inverse[0][1] = s * y[0];
	transform_inverse[0][2] = s * z[0];
	transform_inverse[0][3] = 0;
	transform_inverse[1][0] = s * x[1];
	transform_inverse[1][1] = s * y[1];
	transform_inverse[1][2] = s * z[1];
	transform_inverse[1][3] = 0;
	transform_inverse[2][0] = s * x[2];
	transform_inverse[2][1] = s * y[2];
	transform_inverse[2][2] = s * z[2];
	transform_inverse[2][3] = 0;
	transform_inverse[3][0] = -s * glm::dot(x, position.location);
	transform_inverse[3][1] = -s * glm::dot(y, position.location);
	transform_inverse[3][2] = -s * glm::dot(z, position.location);
	transform_inverse[3][3] = 1;
}
