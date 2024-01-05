#version 460 core


layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inColor;


out vec4 vertColor; 


void main() {
	gl_Position = inPosition;
	vertColor = inColor;
}