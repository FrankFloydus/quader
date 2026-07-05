$input a_position, a_normal
$output v_normal

#include <bgfx_shader.sh>

uniform mat4 u_pbrModel;

void main()
{
    v_normal = mul(u_pbrModel, vec4(a_normal, 0.0)).xyz;
    gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
}

