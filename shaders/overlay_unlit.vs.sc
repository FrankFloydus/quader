$input a_position
$output v_gridWorldPosition, v_gridLocalPosition

#include <bgfx_shader.sh>

uniform vec4 u_gridPlaneSize;

void main()
{
    vec3 localPosition = vec3(
        a_position.x * u_gridPlaneSize.x,
        0.0,
        a_position.z * u_gridPlaneSize.y);
    vec4 worldPosition = mul(u_model[0], vec4(a_position, 1.0));
    v_gridWorldPosition = worldPosition.xyz;
    v_gridLocalPosition = localPosition;
    gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
}
