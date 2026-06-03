$input v_uv

#include <bgfx_shader.sh>

uniform vec4 u_time;

void main() {
	float t = u_time.x;

	float r = sin(v_uv.x * 10.0 + t) * 0.5 + 0.5;
	float g = sin(v_uv.y * 10.0 + t * 1.5) * 0.5 + 0.5;
	float b = sin((v_uv.x + v_uv.y) * 10.0 + t) * 0.5 + 0.5;

	gl_FragColor = vec4(r, g, b, 1.0);
}
