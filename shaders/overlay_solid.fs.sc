#include <bgfx_shader.sh>

uniform vec4 u_lineColor;

void main()
{
	gl_FragColor = vec4(u_lineColor.rgb * u_lineColor.a, u_lineColor.a);
}
