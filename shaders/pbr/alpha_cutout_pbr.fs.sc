$input v_normal

#include <bgfx_shader.sh>

uniform vec4 u_pbrBaseColor;
uniform vec4 u_pbrEmissive;
uniform vec4 u_pbrFactors;
uniform vec4 u_pbrUvTransform0;
uniform vec4 u_pbrFlags;

void main()
{
    if (u_pbrBaseColor.a < u_pbrFactors.w) {
        discard;
    }

    vec3 normal = normalize(v_normal);
    vec3 light = normalize(vec3(0.45, 0.80, 0.38));
    float diffuse = max(dot(normal, light), 0.0);
    vec3 emissive = u_pbrEmissive.rgb * u_pbrEmissive.a;
    gl_FragColor = vec4(u_pbrBaseColor.rgb * (0.16 + diffuse * 0.84) + emissive, 1.0);
}

