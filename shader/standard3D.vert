#version 330 core

in vec3 coord3D;
in vec2 tex2D;
uniform mat4 projFrom3D;

void main(void) {	
	gl_Position =  projFrom3D * vec4(coord3D, 1);
}
