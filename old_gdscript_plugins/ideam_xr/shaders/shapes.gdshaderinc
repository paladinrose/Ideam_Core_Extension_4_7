const float TWO_PI = 6.28318530718;

float polygon(vec2 uv, float width, int sides)
{
	uv = uv  * 2.0 - 1.0;

	float angle = atan(uv.x, uv.y);
	float radius = TWO_PI / float(sides);

	float dist = cos(floor(0.5 + angle / radius) * radius - angle) * length(uv);
	float poly = step(width, dist);
	return poly;
}

float circle(vec2 uv, float radius, float feather)
{
	vec2 pos = uv - vec2(0.5);
	float l = length(pos);
	return smoothstep(radius-feather, radius, l);
}

float cheap_circle(vec2 uv, float radius, float feather)
{
	return smoothstep(radius-feather, radius, dot(uv, uv) * 4.0);
}

float ellipse(vec2 uv, vec2 size, float feather)
{

	float shrink_width = 2.0 / size.x;
	float shrink_height = 2.0 / size.y;
	float dist = distance(vec2(uv.x * shrink_width, uv.y * shrink_height), vec2(0.5 * shrink_width, 0.5 * shrink_height));
	float ell = 1.0 - floor(dist);

	return smoothstep(dist, dist + feather, ell);
}

float swirl(vec2 uv, float size, int arms)
{
	float angle = atan(-uv.y + 0.5, uv.x - 0.5) ;
	float len = length(uv - vec2(0.5, 0.5));

	return sin(len * size + angle * float(arms));
}