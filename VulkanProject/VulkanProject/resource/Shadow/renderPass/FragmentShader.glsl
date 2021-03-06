#version 450
layout(location = 0) in vec3 normal;
layout(location = 1) in vec4 worldPos;

layout(set=1,binding=0) uniform sampler2D shadowMap;
layout(set=2,binding=0) uniform ToLight
{
	mat4 toLight;
	vec4 lightDir;
} tl;

layout(location = 0) out vec4 outColor;

#define shadowMapSize 2048.0

void main()
{
	vec4 lightSpacePos = tl.toLight * worldPos;
	lightSpacePos /= lightSpacePos.w;

	float texelSize = 1.0 / shadowMapSize;
	vec2 samplePos = vec2((lightSpacePos.x + 1) * 0.5, (lightSpacePos.y + 1) * 0.5);
	vec2 frac = vec2(samplePos.x * shadowMapSize - floor(samplePos.x * shadowMapSize), samplePos.y * shadowMapSize - floor(samplePos.y * shadowMapSize));
	vec2 flooredSamplePos = vec2(samplePos.x - frac.x / shadowMapSize, samplePos.y - frac.y / shadowMapSize);
	float sample1 = float((texture(shadowMap, vec2(flooredSamplePos.x, flooredSamplePos.y)).r - lightSpacePos.z) > -0.01);
	float sample2 = float((texture(shadowMap, vec2(flooredSamplePos.x + texelSize, flooredSamplePos.y)).r - lightSpacePos.z) > -0.01);
	float sample3 = float((texture(shadowMap, vec2(flooredSamplePos.x, flooredSamplePos.y + texelSize)).r - lightSpacePos.z) > -0.01);
	float sample4 = float((texture(shadowMap, vec2(flooredSamplePos.x + texelSize, flooredSamplePos.y + texelSize)).r - lightSpacePos.z) > -0.01);
	float finalSample = (sample1 * (1.0 - frac.x) + sample2 * frac.x) * (1.0 - frac.y) +
		(sample3 * (1.0 - frac.x) + sample4 * frac.x) * frac.y;

	vec4 colorFromLight = vec4(0.0, 0.6, 0.7, 1.0) * finalSample;

	float angleAttenuation = max(dot(normal, tl.lightDir.xyz), 0.0);
	float rangeAttenuation = max(1.0 - lightSpacePos.z, 0.3);

	outColor = colorFromLight * angleAttenuation * rangeAttenuation + vec4(0.05, 0.03, 0.03, 1.0) * max(dot(normal, vec3(1,0,0)), 0) + vec4(0.05, 0.03, 0.03, 1.0);
	//outColor = vec4(1) * texture(shadowMap, vec3(samplePos.x, samplePos.y, 0), 0.1f);
	//outColor = vec4(1) * texture(shadowMap, vec2(samplePos.x, samplePos.y)).r;
}

// Sampling with sampler2DShadow
/*float sample1 = float((texture(shadowMap, vec3(flooredSamplePos.x, flooredSamplePos.y, 0), 0.1f) - lightSpacePos.z) > -0.01);
float sample2 = float((texture(shadowMap, vec3(flooredSamplePos.x + texelSize, flooredSamplePos.y, 0), 0.1f) - lightSpacePos.z) > -0.01);
float sample3 = float((texture(shadowMap, vec3(flooredSamplePos.x, flooredSamplePos.y + texelSize, 0), 0.1f) - lightSpacePos.z) > -0.01);
float sample4 = float((texture(shadowMap, vec3(flooredSamplePos.x + texelSize, flooredSamplePos.y + texelSize, 0), 0.1f) - lightSpacePos.z) > -0.01);*/