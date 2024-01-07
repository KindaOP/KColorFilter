//#version 460 core
//
//
//layout(location = 0) in vec4 inPosition;
//layout(location = 1) in vec3 inTexCoord;
//layout(location = 2) in vec4 inColor;
//
//
//layout(location = 0) out vec3 vertTexCoord;
//layout(location = 1) out vec4 vertColor; 
//
//
//void main() {
//	gl_Position = inPosition;
//	vertTexCoord = inTexCoord;
//	vertColor = inColor;
//}


#version 460 core


layout(location = 0) out vec4 tmpColor; 


vec4 position[3] = vec4[](
	vec4(0.0f, -0.5f, 0.0f, 1.0f),
	vec4(0.5f, 0.5f, 0.0f, 1.0f),
	vec4(-0.5f, 0.5f, 0.0f, 1.0f)
);

vec4 color[3] = vec4[](
	vec4(1.0f, 0.0f, 0.0f, 1.0f),
	vec4(0.0f, 1.0f, 0.0f, 1.0f),
	vec4(0.0f, 0.0f, 1.0f, 1.0f)
);


void main() {
	gl_Position = position[gl_VertexIndex];
	tmpColor = color[gl_VertexIndex];
}