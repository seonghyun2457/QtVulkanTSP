#version 450

layout(location = 0) in vec2 fragUv;

layout(location = 0) out vec4 outColor; // Final output color (must also have location)

layout(std140, push_constant) uniform PushConstants {
	vec4 color;
	vec4 borderColor;
	vec4 rect;          // xy = center, wz = half width/height size
	float borderWidth;
} pc;

void main() 
{
	// fwidth(): the sum of the absolute values of the approximate partial derivatives of a value
	// with respect to the window-space X and Y coordinates
	vec2 fw = fwidth(fragUv);

	// pixel size of rectangle
	vec2 rectSizePx = 1.f / fw;

	float refPx = min(rectSizePx.x, rectSizePx.y);

	// thikness of border at pixel level
	float borderPx = pc.borderWidth * refPx;


	// UV distance to the closest outout
	float distUvX = min(fragUv.x, 1.f - fragUv.x);
	float distUvY = min(fragUv.y, 1.f - fragUv.y);

	// convert distance unit: UV -> pixel
	float distPxX = distUvX / fw.x;
	float distPxY = distUvY / fw.y;

	float minDistPx = min(distPxX, distPxY);


	// Anti-aliased border edge: smooth transition instead of a hard cutoff
	float aa = smoothstep(borderPx - 0.5, borderPx + 0.5, minDistPx);
	outColor = mix(pc.borderColor, pc.color, aa);
}