vec4 multiply(vec4 base, vec4 blend)
{
	return base * blend;
}

vec4 screen(vec4 base, vec4 blend)
{
	return 1.0 - (1.0 - base) * (1.0 - blend);
}

vec4 darken(vec4 base, vec4 blend)
{
	return min(base, blend);
}

vec4 lighten(vec4 base, vec4 blend)
{
	return max(base, blend);
}

vec4 difference(vec4 base, vec4 blend)
{
	return abs(base - blend);
}

vec4 exclusion(vec4 base, vec4 blend)
{
	return base + blend - 2.0 * base * blend;
}

vec4 overlay(vec4 base, vec4 blend)
{
	vec4 limit = step(0.5, base);
	return mix(2.0 * base * blend, 1.0 - 2.0 * (1.0 - base) * (1.0 - blend), limit);
}

vec4 hard_light(vec4 base, vec4 blend)
{
	vec4 limit = step(0.5, blend);
	return mix(2.0 * base * blend, 1.0 - 2.0 * (1.0 - base) * (1.0 - blend), limit);
}

vec4 soft_light(vec4 base, vec4 blend)
{
	vec4 limit = step(0.5, blend);
	return mix(2.0 * base * blend + base * base * (1.0 - 2.0 * blend), sqrt(base) * (2.0 * blend - 1.0) + (2.0 * base) * (1.0 - blend), limit);
}

vec4 color_dodge(vec4 base, vec4 blend)
{
	return base / (1.0 - blend);
}

vec4 linear_dodge(vec4 base, vec4 blend)
{
	return base + blend;
}

vec4 color_burn(vec4 base, vec4 blend)
{
	return 1.0 - (1.0 - base) / blend;
}

vec4 normal(vec4 base, vec4 blend)
{
	float final_alpha = blend.a + base.a * (1.0 - blend.a);
	if (final_alpha <= 0.00001) {
		return vec4(0.0);
	}
	return vec4((blend.rgb * blend.a + base.rgb * base.a * (1.0 - blend.a)) / final_alpha, final_alpha);
}
