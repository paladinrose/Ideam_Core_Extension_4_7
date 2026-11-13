vec4 blend_multiply(vec4 base, vec4 blend){
	return base * blend;
}

vec4 blend_screen(vec4 base, vec4 blend){
	return 1.0 - (1.0 - base) * (1.0 - blend);
}

vec4 blend_darken(vec4 base, vec4 blend){
	base.xyz = min(base.xyz, blend.xyz);
	return base;
}

vec4 blend_lighten(vec4 base, vec4 blend){
	return max(base, blend);
}

vec4 blend_difference(vec4 base, vec4 blend){
	return abs(base - blend);
}

vec4 blend_exclusion(vec4 base, vec4 blend){
	return base + blend - 2.0 * base * blend;
}

vec4 blend_overlay(vec4 base, vec4 blend){
	vec4 limit = step(0.5, base);
	return mix(2.0 * base * blend, 1.0 - 2.0 * (1.0 - base) * (1.0 - blend), limit);
}

vec4 blend_hard_light(vec4 base, vec4 blend){
	vec4 limit = step(0.5, blend);
	return mix(2.0 * base * blend, 1.0 - 2.0 * (1.0 - base) * (1.0 - blend), limit);
}

vec4 blend_soft_light(vec4 base, vec4 blend){
	vec4 limit = step(0.5, blend);
	return mix(2.0 * base * blend + base * base * (1.0 - 2.0 * blend), sqrt(base) * (2.0 * blend - 1.0) + (2.0 * base) * (1.0 - blend), limit);
}

vec4 blend_color_dodge(vec4 base, vec4 blend){
	return base / (1.0 - blend);
}

vec4 blend_linear_dodge(vec4 base, vec4 blend){
	return base + blend;
}

vec4 blend_linear_burn (vec4 base, vec4 blend){
	return base + blend - 1.0;
}

vec4 blend_color_burn(vec4 base, vec4 blend){
	return 1.0 - (1.0 - base) / blend;
}

vec2 polar_coordinates(vec2 uv, vec2 center, float zoom, float repeat)
{
	vec2 dir = uv - center;
	float radius = length(dir) * 2.0;
	float angle = atan(dir.y, dir.x) * 1.0/(3.1416 * 2.0);
	return mod(vec2(radius * zoom, angle * repeat), 1.0);
}