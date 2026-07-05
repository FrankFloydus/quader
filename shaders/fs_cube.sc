$input v_normal

#include <bgfx_shader.sh>

uniform vec4 u_lightDir;
uniform vec4 u_baseColor;

void main()
{
    vec3 normal = normalize(v_normal);
    vec3 light = normalize(-u_lightDir.xyz);
    float diffuse = max(dot(normal, light), 0.0);
    float intensity = 0.18 + diffuse * 0.82;
    gl_FragColor = vec4(u_baseColor.rgb * intensity, u_baseColor.a);
}

