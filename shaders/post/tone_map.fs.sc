$input v_texcoord0

#include <bgfx_shader.sh>
#include "common/color_space.sh"

SAMPLER2D(s_hdrSceneColor, 0);
uniform vec4 u_toneMapParams;

vec3 crimsonAgxApprox(vec3 color)
{
    return clamp(vec3(1.0, 1.0, 1.0) - exp(-max(color, vec3(0.0, 0.0, 0.0)) * 0.84), 0.0, 1.0);
}

vec3 crimsonReinhard(vec3 color)
{
    vec3 safeColor = max(color, vec3(0.0, 0.0, 0.0));
    return clamp(safeColor / (safeColor + vec3(1.0, 1.0, 1.0)), 0.0, 1.0);
}

void main()
{
    vec4 hdr = texture2D(s_hdrSceneColor, v_texcoord0);
    float exposureMultiplier = max(u_toneMapParams.y, 0.0);
    vec3 exposed = hdr.rgb * exposureMultiplier;
    vec3 linearSdr = clamp(exposed, 0.0, 1.0);
    if (u_toneMapParams.x < 0.5) {
        linearSdr = crimsonAcesFitted(exposed);
    } else if (u_toneMapParams.x < 1.5) {
        linearSdr = crimsonAgxApprox(exposed);
    } else if (u_toneMapParams.x < 2.5) {
        linearSdr = crimsonReinhard(exposed);
    }
    gl_FragColor = vec4(linearSdr, hdr.a);
}
