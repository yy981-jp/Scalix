$input a_position, a_texcoord0, a_weight, a_indices
$output v_texcoord0

#include <bgfx_shader.sh>

uniform mat4 u_boneMatrices[100];

void main()
{
    vec4 pos = vec4(a_position, 1.0);
    vec4 skinned = vec4(0.0, 0.0, 0.0, 0.0);

    for (int i = 0; i < 4; i++) {
        int idx = int(a_indices[i]);
        float w = a_weight[i];

        if (w > 0.0) {
            skinned += mul(u_boneMatrices[idx], pos) * w;
        }
    }

    gl_Position = mul(u_modelViewProj, skinned);
    v_texcoord0 = a_texcoord0;
}