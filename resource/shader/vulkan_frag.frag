#version 460 core


layout(location = 0) in vec3 vertTexCoord;
layout(location = 1) in vec4 vertColor; 


layout(location = 0) out vec4 fragColor;


void main() {
	fragColor = vertColor;
}