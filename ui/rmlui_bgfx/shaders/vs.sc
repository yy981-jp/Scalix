$input a_position, a_color0, a_texcoord0
$output v_color0, v_texcoord0

#include <bgfx_shader.sh>

uniform mat4 u_projection;
uniform mat4 u_transform;
uniform vec4 u_translate;

void main()
{
	vec4 position = vec4(a_position + u_translate.xy, 0.0, 1.0);
	gl_Position = mul(u_projection, mul(u_transform, position));
	v_color0 = a_color0;
	v_texcoord0 = a_texcoord0;
}
