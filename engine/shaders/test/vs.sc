$input a_position
$output v_uv

#include <bgfx_shader.sh>

void main() {
	gl_Position = vec4(a_position, 1.0);
	v_uv = a_position.xy * 0.5 + 0.5;
}
