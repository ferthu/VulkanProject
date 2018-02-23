#version 450
layout(location=0) in vec4 position_in;

void main()
{
	gl_Position = position_in;
}
