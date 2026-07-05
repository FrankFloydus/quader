$input v_normal, v_texcoord0

#include <bgfx_shader.sh>

uniform vec4 u_pbrBaseColor;
uniform vec4 u_pbrEmissive;
uniform vec4 u_pbrFactors;
uniform vec4 u_pbrUvTransform0;
uniform vec4 u_pbrFlags;
SAMPLER2D(s_pbrBaseColorTexture, 0);

void main()
{
    vec3 normal = normalize(v_normal);
    vec3 light = normalize(vec3(0.45, 0.80, 0.38));
    float diffuse = max(dot(normal, light), 0.0);
    float roughness = clamp(u_pbrFactors.y, 0.04, 1.0);
    float intensity = 0.16 + diffuse * (0.88 - roughness * 0.16);
    vec3 emissive = u_pbrEmissive.rgb * u_pbrEmissive.a;
    vec2 uv0 = vec2(v_texcoord0.x, 1.0 - v_texcoord0.y) * u_pbrUvTransform0.xy + u_pbrUvTransform0.zw;
    vec4 baseColorTex = texture2D(s_pbrBaseColorTexture, uv0);
    vec4 baseColor = u_pbrBaseColor * baseColorTex;
    gl_FragColor = vec4(baseColor.rgb * intensity + emissive, baseColor.a);
}
