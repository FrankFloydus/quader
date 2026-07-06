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
    gl_FragColor = vec4(baseColor.rgb * (0.16 + diffuse * 0.84) + emissive, 1.0);
}
