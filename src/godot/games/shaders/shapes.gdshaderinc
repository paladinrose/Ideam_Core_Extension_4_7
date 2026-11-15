float circle(vec2 position, float radius)
{
	return step(radius, length(position - vec2(0.5)));
}

float feathered_circle(vec2 position, vec2 center, float radius, float feather)
{
	float l = length(position - center);
	if (l == 0.0) {
		return 0.0;
	}
	return smoothstep(radius, radius + feather, l);
}
