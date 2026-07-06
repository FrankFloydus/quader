$input v_gridWorldPosition, v_gridLocalPosition

#include <bgfx_shader.sh>

uniform vec4 u_gridColor;
uniform vec4 u_majorGridColor;
uniform vec4 u_originUColor;
uniform vec4 u_originVColor;
uniform vec4 u_cameraPosition;
uniform vec4 u_gridUAxis;
uniform vec4 u_gridVAxis;
uniform vec4 u_gridParams0;
uniform vec4 u_gridParams1;
uniform vec4 u_gridParams2;
uniform vec4 u_gridParams3;

float gridLine(float coord, float spacing, float widthScale, float thicknessValue)
{
    float safeSpacing = max(spacing, 0.0001);
    float wrapped = abs(fract(coord / safeSpacing + 0.5) - 0.5) * safeSpacing;
    float thickness = clamp(thicknessValue, 0.05, 8.0);
    float antialias = max(fwidth(coord), safeSpacing * 0.001);
    float lineWidth = antialias * max(widthScale, 0.05) * thickness;
    float coverage = 1.0 - smoothstep(lineWidth, lineWidth + antialias, wrapped);
    return coverage * min(thickness, 1.0);
}

float axisLine(float coord)
{
    float thickness = clamp(u_gridParams1.x, 0.05, 8.0);
    float antialias = max(fwidth(coord), 0.0001);
    float lineWidth = antialias * 0.9 * thickness;
    float coverage = 1.0 - smoothstep(lineWidth, lineWidth + antialias, abs(coord));
    return coverage * min(thickness, 1.0);
}

vec3 safeNormalize(vec3 value, vec3 fallback)
{
    float len = length(value);
    if (len <= 0.000001) {
        return fallback;
    }
    return value / len;
}

void main()
{
    vec3 worldPosition = v_gridWorldPosition;
    vec3 safeUAxis = safeNormalize(u_gridUAxis.xyz, vec3(1.0, 0.0, 0.0));
    vec3 safeVAxis = safeNormalize(u_gridVAxis.xyz, vec3(0.0, 0.0, -1.0));
    vec2 coords = vec2(dot(worldPosition, safeUAxis), dot(worldPosition, safeVAxis));
    vec2 cameraCoords = vec2(
        dot(u_cameraPosition.xyz, safeUAxis),
        dot(u_cameraPosition.xyz, safeVAxis));

    float baseSpacing = max(u_gridParams0.x, 0.0001);
    float gridValue = max(
        gridLine(coords.x, baseSpacing, 0.65, u_gridParams0.z),
        gridLine(coords.y, baseSpacing, 0.65, u_gridParams0.z));
    float parentSpacing = max(max(u_gridParams0.y, 0.0001), baseSpacing);
    float parentGridValue = max(
        gridLine(coords.x, parentSpacing, 0.85, u_gridParams0.w),
        gridLine(coords.y, parentSpacing, 0.85, u_gridParams0.w));

    float distanceToCamera = length(coords - cameraCoords);
    float distanceFade = 1.0 - smoothstep(u_gridParams1.y, u_gridParams1.z, distanceToCamera);
    float worldCameraDistance = length(worldPosition - u_cameraPosition.xyz);
    float farHalf = max(u_gridParams3.x * 0.5, 0.0001);
    float farClipFade = 1.0 - smoothstep(0.0, farHalf, worldCameraDistance - farHalf);
    farClipFade = mix(1.0, farClipFade, clamp(u_gridParams3.y, 0.0, 1.0));
    float edgeDistance = max(abs(v_gridLocalPosition.x), abs(v_gridLocalPosition.z));
    float edgeFade = 1.0 - smoothstep(u_gridParams1.w * 0.88, u_gridParams1.w, edgeDistance);
    float fade = clamp(distanceFade * edgeFade * farClipFade, 0.0, 1.0);
    fade += (u_gridParams2.x + u_gridParams2.y + u_gridParams2.z + u_gridParams2.w) * 0.0;

    float majorCoverage = parentGridValue * u_majorGridColor.a;
    float minorCoverage = gridValue * u_gridColor.a * (1.0 - clamp(majorCoverage, 0.0, 1.0));
    float alpha = minorCoverage + majorCoverage;
    vec3 color = u_gridColor.rgb;
    if (alpha > 0.0) {
        color = (u_gridColor.rgb * minorCoverage + u_majorGridColor.rgb * majorCoverage) / alpha;
    }

    float uAxis = axisLine(coords.y);
    float vAxis = axisLine(coords.x);
    if (uAxis > 0.0 && u_originUColor.a > 0.0) {
        color = mix(color, u_originUColor.rgb, uAxis);
        alpha = max(alpha, uAxis * u_originUColor.a);
    }
    if (vAxis > 0.0 && u_originVColor.a > 0.0) {
        color = mix(color, u_originVColor.rgb, vAxis);
        alpha = max(alpha, vAxis * u_originVColor.a);
    }

    alpha = clamp(alpha * fade, 0.0, 1.0);
    if (alpha < 0.002) {
        discard;
    }

    gl_FragColor = vec4(color * alpha, alpha);
}
