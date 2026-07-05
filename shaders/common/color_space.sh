vec3 crimsonAcesFitted(vec3 color)
{
    vec3 safeColor = max(color, vec3(0.0, 0.0, 0.0));
    vec3 a = vec3(2.51, 2.51, 2.51);
    vec3 b = vec3(0.03, 0.03, 0.03);
    vec3 c = vec3(2.43, 2.43, 2.43);
    vec3 d = vec3(0.59, 0.59, 0.59);
    vec3 e = vec3(0.14, 0.14, 0.14);
    return clamp((safeColor * (a * safeColor + b)) / (safeColor * (c * safeColor + d) + e), 0.0, 1.0);
}

vec3 crimsonLinearToSrgb(vec3 color)
{
    vec3 safeColor = clamp(color, 0.0, 1.0);
    vec3 low = safeColor * 12.92;
    vec3 high = 1.055 * pow(safeColor, vec3(1.0 / 2.4, 1.0 / 2.4, 1.0 / 2.4)) - vec3(0.055, 0.055, 0.055);
    vec3 selector = step(vec3(0.0031308, 0.0031308, 0.0031308), safeColor);
    return mix(low, high, selector);
}
