shader_type spatial;
render_mode blend_mix, depth_test_disabled, skip_vertex_transform, unshaded, cull_disabled;

uniform vec4 color : source_color = vec4(0.0, 0.0, 0.0, 1.0);
uniform sampler2D albedo : hint_default_white;

void vertex() {
  POSITION = vec4(VERTEX, 1.0);
}

void fragment() {

	vec4 tex = texture(albedo, UV) * color;
	ALBEDO = tex.xyz;
	ALPHA = tex.w;
}
