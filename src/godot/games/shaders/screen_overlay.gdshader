shader_type canvas_item;

#include "res://src/godot/games/shaders/shapes.gdshaderinc"
#include "res://src/godot/games/shaders/blending.gdshaderinc"

uniform vec4 color : source_color = vec4(0.0, 0.0, 0.0, 0.0);
uniform sampler2D albedo : source_color;

uniform float override : hint_range(0.0, 1.0) = 0.0;
uniform vec4 override_color : source_color = vec4(0.0, 0.0, 0.0, 0.0);
uniform sampler2D override_albedo : source_color;

uniform float eye_open : hint_range(0.0, 1.0) = 1.0;
uniform float eye_feather : hint_range(0.0, 1.0) = 0.0;
uniform vec4 eye_closed_color : source_color = vec4(0.0, 0.0, 0.0, 1.0);
uniform sampler2D eye_closed_texture : source_color;

void fragment() {
	vec4 albedo_tex = texture(albedo, UV);
	vec4 col = normal(albedo_tex, color);

	vec4 o_albedo_tex = texture(override_albedo, UV);
	vec4 o_col = normal(o_albedo_tex, override_color);

	vec4 final_col = mix(col, o_col, override);

	vec4 closed_tex = texture(eye_closed_texture, UV);
	vec4 closed_col = normal(closed_tex, eye_closed_color);

	float e_open = clamp(eye_open, 0.0, 1.0);
	float e_feather = clamp(eye_feather, 0.0, 1.0 - e_open);
	float circle = feathered_circle(UV, vec2(0.5), e_open, e_feather);

	final_col = mix(final_col, closed_col, circle);

	COLOR = final_col;
}
