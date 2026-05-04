$input a_position, a_texcoord0, a_weight, a_indices
$output v_texcoord0

#include <bgfx_shader.sh>

uniform mat4 u_boneMatrices[120];

void main() {
    float4 pos = float4(a_position, 1.0);

    float3 skinned = float3(0.0, 0.0, 0.0);

    int4 joints = int4(a_indices + 0.5);
    float4 weights = a_weight;

    skinned += mul(u_boneMatrices[joints.x], pos).xyz * weights.x;
    skinned += mul(u_boneMatrices[joints.y], pos).xyz * weights.y;
    skinned += mul(u_boneMatrices[joints.z], pos).xyz * weights.z;
    skinned += mul(u_boneMatrices[joints.w], pos).xyz * weights.w;

    float sum = weights.x + weights.y + weights.z + weights.w;
    if (sum > 0.0001)
    {
        skinned /= sum;
    }

    gl_Position = mul(u_modelViewProj, float4(skinned, 1.0));

    v_texcoord0 = a_texcoord0;

}
