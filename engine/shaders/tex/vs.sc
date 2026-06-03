$input a_position, a_texcoord0, a_indices, a_weight
$output v_texcoord0

#include <bgfx_shader.sh>

uniform mat4 u_boneMatrices[120];

void main()
{
	mat4 skin =
		a_weight.x * u_boneMatrices[int(a_indices.x)] +
		a_weight.y * u_boneMatrices[int(a_indices.y)] +
		a_weight.z * u_boneMatrices[int(a_indices.z)] +
		a_weight.w * u_boneMatrices[int(a_indices.w)];

	gl_Position = mul(
		u_modelViewProj,
		mul(skin, vec4(a_position, 1.0))
	);

	v_texcoord0 = a_texcoord0;
}
