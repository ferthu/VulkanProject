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
	vec4 windowSpacePos = vec4(gl_FragCoord.x / 800.0f, gl_FragCoord.y / 600.0f, gl_FragCoord.z, 1);
	
	vec4 lightSpacePos = tl.toLight * windowSpacePos;
	lightSpacePos /= lightSpacePos.w;
	outColor = vec4(1.0, 1.0, 1.0, 1.0) * texture(shadowMap, lightSpacePos.xyz, 0.01f) + vec4(0.1, 0.1, 0.1, 0.1);
	
	outColor = lightSpacePos;
}
