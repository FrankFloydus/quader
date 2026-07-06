$input v_lineDistancePixels

#include <bgfx_shader.sh>

uniform vec4 u_lineColor;
uniform vec4 u_lineParams;

float lineCoverage(float distancePixels)
{
	float lineWidth = max(u_lineParams.x, 1.0);
	float featherPixels = max(u_lineParams.y, 0.0);
	float halfWidth = lineWidth * 0.5;
	if (featherPixels <= 0.0001) {
		return step(distancePixels, halfWidth);
	}
	float outerDistance = (lineWidth + featherPixels) * 0.5;
	return 1.0 - smoothstep(halfWidth, outerDistance, distancePixels);
}

void main()
{
	float coverage = clamp(lineCoverage(abs(v_lineDistancePixels)), 0.0, 1.0);
	float alpha = u_lineColor.a * coverage;
	gl_FragColor = vec4(u_lineColor.rgb * alpha, alpha);
}
