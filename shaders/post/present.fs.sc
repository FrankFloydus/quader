$input v_texcoord0

#include <bgfx_shader.sh>
#include "common/color_space.sh"

SAMPLER2D(s_toneMappedColor, 0);
uniform vec4 u_presentParams;

void main()
{
    vec4 linearSdr = texture2D(s_toneMappedColor, v_texcoord0);
    vec3 displayColor = linearSdr.rgb;
    if (u_presentParams.x > 0.5) {
        displayColor = crimsonLinearToSrgb(displayColor);
    }
    gl_FragColor = vec4(displayColor, linearSdr.a);
}
