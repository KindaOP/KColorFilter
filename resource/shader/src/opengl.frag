#version 460 core


in vec3 vertTexCoord;
in vec4 vertColor; 


uniform sampler2DArray textures;


void main() {
	if (vertTexCoord[2] < 0.0f) {
		gl_FragColor = vertColor;
	}
	else {
		gl_FragColor = texture(textures, vertTexCoord);
	}
}