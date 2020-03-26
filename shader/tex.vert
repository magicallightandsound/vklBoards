#version 330 core

layout(location = 0) in vec3 coord3D;
layout(location = 1) in vec2 tex2D;
uniform mat4 projFrom3D;
out vec2 texcoord;

void main(){
	gl_Position =  projFrom3D * vec4(coord3D, 1);
    texcoord = tex2D;
}