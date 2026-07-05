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
    vec3 emissive = u_pbrEmissive.rgb * u_pbrEmissive.a;
    vec2 uv0 = vec2(v_texcoord0.x, 1.0 - v_texcoord0.y) * u_pbrUvTransform0.xy + u_pbrUvTransform0.zw;
    vec4 baseColorTex = texture2D(s_pbrBaseColorTexture, uv0);
    vec4 baseColor = u_pbrBaseColor * baseColorTex;
    gl_FragColor = vec4(baseColor.rgb + emissive, baseColor.a);
}
