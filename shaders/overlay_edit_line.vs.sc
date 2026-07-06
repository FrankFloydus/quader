$input a_position, a_texcoord0
$output v_worldPosition, v_texcoord0

#include <bgfx_shader.sh>

void main()
{
	v_worldPosition = vec3(a_position.xy, a_texcoord0.y);
	v_texcoord0 = vec2(a_texcoord0.x, 0.0);
	gl_Position = vec4(a_position, 1.0);
}
