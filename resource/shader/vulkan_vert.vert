#version 460 core


layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec3 inTexCoord;
layout(location = 2) in vec4 inColor;


layout(location = 0) out vec3 vertTexCoord;
layout(location = 1) out vec4 vertColor; 


void main() {
	gl_Position = inPosition;
	vertTexCoord = inTexCoord;
	vertColor = inColor;
}