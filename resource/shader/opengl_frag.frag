#version 460 core


uniform sampler2DArray textures;


in vec3 vertTexCoord;
in vec4 vertColor; 


out vec4 fragColor;


void main() {
	if (vertTexCoord[2] < 0.0f) {
		fragColor = vertColor;
	}
	else {
		fragColor = texture(textures, vertTexCoord);
	}
}