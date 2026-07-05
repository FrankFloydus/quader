$input v_normal

#include <bgfx_shader.sh>

uniform vec4 u_pickingId;

void main()
{
    gl_FragColor = u_pickingId + vec4(v_normal * 0.0, 0.0);
}

