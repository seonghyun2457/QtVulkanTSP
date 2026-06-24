#version 450 // Use GLSL 4.5

layout(location = 0) in vec3 pos; // Local unit-quad position in [-1, 1]
layout(location = 1) in vec2 uv;

layout (std140, set = 0, binding = 0) uniform UboModelViewProjection {
	mat4 model;
	mat4 view;
	mat4 projection;
} uboModelViewProjection;

layout(std140, push_constant) uniform PushConstants {
	vec4 color;
	vec4 borderColor;
	vec4 rect;          // xy = center (NDC), wz = half width/height size (NDC)
	float borderWidth;
} pc;

layout(location = 0) out vec2 fragUv;

void main() 
{
	vec2 worldPos = pos.xy * pc.rect.zw + pc.rect.xy;
	gl_Position = uboModelViewProjection.projection * uboModelViewProjection.view * uboModelViewProjection.model * vec4(worldPos, 0.0, 1.0);

	fragUv = uv;
}