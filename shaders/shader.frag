#version 450

layout(location = 0) in vec3 fragCol;

layout(location = 0) out vec4 outColor; // Final output color (must also have location)

layout(push_constant) uniform PushConstants {
	vec4 color;
} pc;

void main() 
{
	outColor = pc.color;
}