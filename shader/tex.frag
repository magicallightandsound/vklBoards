#version 330 core

in vec2 texcoord;
uniform vec4 color;
out vec4 fragColor;
uniform sampler2D tex;

void main(void) {
	fragColor = (1-color.a) * texture(tex, texcoord) + color.a * vec4(color.rgb, 0);
}
