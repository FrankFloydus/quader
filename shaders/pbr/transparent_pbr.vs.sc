$input a_position, a_normal, a_texcoord0
$output v_normal, v_worldPosition, v_texcoord0

#include <bgfx_shader.sh>

uniform mat4 u_pbrModel;

void main()
{
    vec4 worldPosition = mul(u_pbrModel, vec4(a_position, 1.0));
    v_worldPosition = worldPosition.xyz;
    v_normal = mul(u_pbrModel, vec4(a_normal, 0.0)).xyz;
    v_texcoord0 = a_texcoord0;
    gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
}
