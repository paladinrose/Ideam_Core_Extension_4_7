shader_type spatial;
render_mode blend_mix, depth_test_disabled, skip_vertex_transform, unshaded, cull_disabled;

#include "res://addons/ideam_xr/shaders/shapes.gdshaderinc"
#include "res://addons/ideam_xr/shaders/blending.gdshaderinc"

uniform int view_id;

uniform vec4 color : source_color = vec4(1.0, 1.0, 1.0, 1.0);
uniform sampler2D albedo : hint_default_white;

uniform float override : hint_range(0.0, 1.0, 0.01) = 0.0;
uniform vec4 override_color : source_color = vec4(1.0, 1.0, 1.0, 1.0);
uniform sampler2D override_albedo : hint_default_white;

uniform float eye_open : hint_range(0.0, 1.0, 0.01) = 1.0;
uniform float eye_feather = 0.0;
uniform vec4 eye_closed_color : source_color = vec4(0.0,0.0,0.0,1.0);
uniform sampler2D eye_closed_texture : hint_default_white;

void vertex() {
  POSITION = vec4(VERTEX.xy, 1.0, 1.0);
}

void fragment() {
	//int vid = VIEW_INDEX;

	//if (view_id < 0){
		//vid = view_id;
	//}

	//if (vid == view_id){
	vec4 tex = texture(albedo, UV) * color;
	vec4 ov = texture(override_albedo, UV) * override_color;

	tex = tex*(1.0-override) + ov * override;

	vec2 eyesize = vec2(1.5, eye_open);
	eyesize *= 1.5;

	vec2 eye_uv = UV;
	eye_uv.y -= 0.1;

	float feather = eye_feather * (1.0-eye_open);
	float eyemask = 1.0 - ellipse(eye_uv, eyesize, feather);

	vec2 windowsize = max(vec2(0.0),eyesize-feather);
	float eyewindow = 1.0 - ellipse(eye_uv, windowsize, 0);

	vec4 ev = texture(eye_closed_texture, UV) * eye_closed_color;
	ev.w = eyemask;
	//tex = blend_darken(tex, ev);
	tex = ev * eyemask + tex * (1.0 - eyewindow);

	ALBEDO = tex.xyz;
	ALPHA = tex.w;
	//}
	//else {
		//ALPHA = 0.0;
	//}
}
