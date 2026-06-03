$input a_position, a_texcoord0, a_indices, a_weight
$output v_texcoord0

#include <bgfx_shader.sh>

uniform mat4 u_bones[100];

void main()
{
	mat4 skin =
		a_weight.x * u_bones[int(a_indices.x + 0.5)] +
		a_weight.y * u_bones[int(a_indices.y + 0.5)] +
		a_weight.z * u_bones[int(a_indices.z + 0.5)] +
		a_weight.w * u_bones[int(a_indices.w + 0.5)];

	gl_Position = mul(
		u_modelViewProj,
		mul(skin, vec4(a_position, 1.0))
	);

	v_texcoord0 = a_texcoord0;
}