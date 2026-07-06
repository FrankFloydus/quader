$input v_normal, v_worldPosition, v_texcoord0

#include <bgfx_shader.sh>

uniform vec4 u_pbrBaseColor;
uniform vec4 u_pbrEmissive;
uniform vec4 u_pbrFactors;
uniform vec4 u_pbrUvTransform0;
uniform vec4 u_pbrFlags;
uniform vec4 u_surfaceGridMinorColor; // surfaceGridMinorColor
uniform vec4 u_surfaceGridMajorColor; // surfaceGridMajorColor
uniform vec4 u_surfaceGridParams; // x surfaceGridSize, y surfaceGridMajorSize, z surfaceGridMinorLineThickness, w surfaceGridMajorLineThickness
uniform vec4 u_surfaceGridFlags; // x surfaceGridEnabled
SAMPLER2D(s_pbrBaseColorTexture, 0);

float surfaceGridFloor(float spacing)
{
    return exp2(floor(log2(max(spacing, 0.0001))));
}

float surfaceGridLineCoverage(float coord, float spacing, float thicknessValue)
{
    float safeSpacing = max(spacing, 0.0001);
    float wrapped = abs(fract(coord / safeSpacing + 0.5) - 0.5) * safeSpacing;
    float thickness = clamp(thicknessValue, 0.05, 8.0);
    float derivativeWidth = fwidth(coord);
    float antialias = clamp(derivativeWidth, safeSpacing * 0.0005, safeSpacing * 0.125);
    float lineWidth = min(antialias * 0.75 * thickness, safeSpacing * 0.125);
    float feather = min(antialias, safeSpacing * 0.125);
    return 1.0 - smoothstep(lineWidth, lineWidth + feather, wrapped);
}

float surfaceGridLine(float coord, float spacing, float thicknessValue, float worldPixelWidth)
{
    float safeSpacing = max(spacing, 0.0001);
    float minRenderableSpacing = max(worldPixelWidth, 0.0) * 2.0;
    float lodSpacing = max(surfaceGridFloor(max(minRenderableSpacing, safeSpacing)), safeSpacing);
    float nextSpacing = max(lodSpacing * 2.0, lodSpacing + 0.0001);
    float blend = smoothstep(lodSpacing, nextSpacing, minRenderableSpacing);
    float currentCoverage = surfaceGridLineCoverage(coord, lodSpacing, thicknessValue);
    float nextCoverage = surfaceGridLineCoverage(coord, nextSpacing, thicknessValue);
    return mix(currentCoverage, nextCoverage, blend);
}

vec2 surfaceGridCoordinates(vec3 worldPosition, vec3 normal)
{
    vec3 dominant = abs(normal);
    if (dominant.x >= dominant.y && dominant.x >= dominant.z) {
        return worldPosition.yz;
    }
    if (dominant.y >= dominant.z) {
        return worldPosition.xz;
    }
    return worldPosition.xy;
}

vec3 applySurfaceGrid(vec3 baseColor, vec3 worldPosition, vec3 worldNormal)
{
    if (u_surfaceGridFlags.x <= 0.5) {
        return baseColor;
    }

    vec2 coords = surfaceGridCoordinates(worldPosition, normalize(worldNormal));
    float worldPixelWidth = max(length(dFdx(worldPosition)), length(dFdy(worldPosition)));
    float baseSpacing = max(u_surfaceGridParams.x, 0.0001);
    float majorSpacing = max(u_surfaceGridParams.y, baseSpacing);
    float minorLine = max(
        surfaceGridLine(coords.x, baseSpacing, u_surfaceGridParams.z, worldPixelWidth),
        surfaceGridLine(coords.y, baseSpacing, u_surfaceGridParams.z, worldPixelWidth));
    float majorLine = max(
        surfaceGridLine(coords.x, majorSpacing, u_surfaceGridParams.w, worldPixelWidth),
        surfaceGridLine(coords.y, majorSpacing, u_surfaceGridParams.w, worldPixelWidth));

    float majorCoverage = majorLine * u_surfaceGridMajorColor.a;
    float minorCoverage = minorLine * u_surfaceGridMinorColor.a * (1.0 - clamp(majorCoverage, 0.0, 1.0));
    float alpha = clamp(minorCoverage + majorCoverage, 0.0, 1.0);
    if (alpha <= 0.000001) {
        return baseColor;
    }

    vec3 gridColor = (u_surfaceGridMinorColor.rgb * minorCoverage + u_surfaceGridMajorColor.rgb * majorCoverage) / alpha;
    return mix(baseColor, gridColor, alpha);
}

void main()
{
    vec2 uv0 = v_texcoord0 * u_pbrUvTransform0.xy + u_pbrUvTransform0.zw;
    vec4 baseColorTex = texture2D(s_pbrBaseColorTexture, uv0);
    vec4 baseColor = u_pbrBaseColor * baseColorTex;
    if (baseColor.a < u_pbrFactors.w) {
        discard;
    }

    vec3 normal = normalize(v_normal);
    vec3 light = normalize(vec3(0.45, 0.80, 0.38));
    float diffuse = max(dot(normal, light), 0.0);
    vec3 emissive = u_pbrEmissive.rgb * u_pbrEmissive.a;
    baseColor.rgb = applySurfaceGrid(baseColor.rgb, v_worldPosition, normal);
    gl_FragColor = vec4(baseColor.rgb * (0.16 + diffuse * 0.84) + emissive, 1.0);
}
