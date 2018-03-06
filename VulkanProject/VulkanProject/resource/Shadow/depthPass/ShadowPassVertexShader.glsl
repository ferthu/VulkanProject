#version 450
layout(location=0) in vec4 position;
layout(location=1) in vec3 normal;

layout(set=0,binding=0) uniform shadowTransform
{
	mat4 transform;
} st;

void main()
{
	gl_Position = position * st.transform;
}