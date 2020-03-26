#pragma once

#include "platform_includes.h"
#include "BoardContent.h"
#include <vector>

class BoardClient;
class Shader;

class Whiteboard : public BoardContent{
	BoardClient *client;
public:
	Whiteboard(BoardClient *client, bool is_remote, int board_index, const std::string &name);
	~Whiteboard();

	struct Position{
		glm::vec3 location;
		glm::vec3 normal;
		float scale;
	};

	std::string name;
	bool is_remote;
	int iboard;
	bool has_placement;
public:
	void ApplyShader(Shader& shader);
	void Render(glm::mat4 projectionMatrix);
	
	// Given a ray o+t*d, returns a point p on the board.
	// Returns 1 if there is an intersection, otherwise, 0.
	int RayIntersection(const glm::vec3 &o, const glm::vec3 &d, glm::vec2 &p, float &t) const;
	
	void ReceiveInput(const glm::vec3 &pos, const glm::quat &rot, bool pressed, const glm::vec2 &cursor_pos);
	
	void set_position(const glm::vec3 &pos, const glm::quat &rot);
	
	void set_visibility(bool state);
	bool get_visibility() const;
	const std::string &get_name() const;
public:
	GLuint _progId;
	GLuint _vaoId;
	GLuint _projId;
	GLuint _texId;

	Position position;
	glm::mat4 transform; // derived from position; updated whenever position is updated
	glm::mat4 transform_inverse;

	bool visible;

	void UpdateTransform();
	void coord2px(const glm::vec2 &p, int &x, int &y) const;
	void px2coord(int x, int y, glm::vec2 &p) const;
	
	void on_image_update(Region *touched = NULL);
public:
	void UpdateTexture(const unsigned char *data, unsigned stride, unsigned x, unsigned y, unsigned w, unsigned h);
};
