#version 330 core

in vec2 texcoord;
out vec4 fragColor;
uniform sampler2D tex;

void main(void) {
	fragColor = texture(tex, texcoord);
}
