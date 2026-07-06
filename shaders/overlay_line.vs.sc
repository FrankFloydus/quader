$input a_position, a_texcoord0
$output v_lineDistancePixels

#include <bgfx_shader.sh>

void main()
{
	v_lineDistancePixels = a_texcoord0.x;
	gl_Position = vec4(a_position, 1.0);
}
