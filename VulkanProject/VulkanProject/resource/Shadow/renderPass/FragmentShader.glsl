#version 450
layout(location = 0) in vec3 normal;

layout(set=1,binding=0) uniform sampler2DShadow shadowMap;
layout(set=1,binding=2) uniform ToLight
{
	mat4 toLight;
} tl;

layout(location = 0) out vec4 outColor;

void main()
{
	vec4 windowSpacePos = gl_FragCoord / gl_FragCoord.w;
	
	vec4 lightSpacePos = windowSpacePos * tl.toLight;
	lightSpacePos /= lightSpacePos.w;
	outColor = vec4(0.8, 0.5, 0.9, 1.0) * texture(shadowMap, lightSpacePos.xyz) + vec4(0.1, 0.1, 0.1, 0.1);
}
