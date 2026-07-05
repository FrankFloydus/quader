$input v_texcoord0

#include <bgfx_shader.sh>
#include "common/color_space.sh"

SAMPLER2D(s_hdrSceneColor, 0);
uniform vec4 u_toneMapParams;

void main()
{
    vec4 hdr = texture2D(s_hdrSceneColor, v_texcoord0);
    float exposureMultiplier = max(u_toneMapParams.y, 0.0);
    vec3 exposed = hdr.rgb * exposureMultiplier;
    vec3 linearSdr = crimsonAcesFitted(exposed);
    gl_FragColor = vec4(linearSdr, hdr.a);
}
