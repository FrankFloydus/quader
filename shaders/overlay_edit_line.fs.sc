$input v_worldPosition, v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_sceneDepth, 0);
uniform vec4 u_lineColor;
uniform vec4 u_lineParams;
uniform vec4 u_editLineDepthParams;
uniform vec4 u_editLineCameraParams;

float packedProjectionMode()
{
	return u_editLineCameraParams.w - step(4.0, u_editLineCameraParams.w) * 4.0;
}

float depthDeviceZ(float sampledDepth)
{
	float mode = packedProjectionMode();
	if ((mode > 0.5 && mode < 1.5) || mode > 2.5) {
		return sampledDepth * 2.0 - 1.0;
	}
	return sampledDepth;
}

float viewDepthFromDeviceZ(float deviceZ)
{
	float nearPlane = max(u_editLineCameraParams.x, 0.000001);
	float farPlane = max(u_editLineCameraParams.y, nearPlane + 0.000001);
	float diff = max(farPlane - nearPlane, 0.000001);
	float mode = packedProjectionMode();
	if (mode > 1.5) {
		if (mode > 2.5) {
			return (deviceZ * diff + nearPlane + farPlane) * 0.5;
		}
		return deviceZ * diff + nearPlane;
	}
	float depthScale = mode > 0.5
			? (farPlane + nearPlane) / diff
			: farPlane / diff;
	float depthBias = mode > 0.5
			? (2.0 * farPlane * nearPlane) / diff
			: (farPlane * nearPlane) / diff;
	return depthBias / max(0.000001, depthScale - deviceZ);
}

vec2 editLineDepthUv(vec2 devicePosition)
{
	float originBottomLeft = step(4.0, u_editLineCameraParams.w);
	float localU = devicePosition.x * 0.5 + 0.5;
	float topLeftV = 0.5 - devicePosition.y * 0.5;
	float bottomLeftV = 0.5 + devicePosition.y * 0.5;
	float localV = mix(topLeftV, bottomLeftV, originBottomLeft);
	return u_editLineDepthParams.xy + vec2(localU, localV) * u_editLineDepthParams.zw;
}

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
	float sceneSampledDepth = texture2D(s_sceneDepth, editLineDepthUv(v_worldPosition.xy)).x;
	float sceneViewDepth = viewDepthFromDeviceZ(depthDeviceZ(sceneSampledDepth));
	float lineViewDepth = viewDepthFromDeviceZ(v_worldPosition.z);
	if (lineViewDepth > sceneViewDepth + max(u_editLineCameraParams.z, 0.0)) {
		discard;
	}

	float coverage = clamp(lineCoverage(abs(v_texcoord0.x)), 0.0, 1.0);
	float alpha = u_lineColor.a * coverage;
	gl_FragColor = vec4(u_lineColor.rgb * alpha, alpha);
}
