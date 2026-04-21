struct VertexOutput {
    float4 position : SV_POSITION;
    float2 uv;
    // float3 normal;
};

[shader("vertex")]
VertexOutput main(uint vertex_index : SV_VulkanVertexID) {
    float2 positions[6] = {
        float2(0., 0.),
        float2(1., 0.),
        float2(1., 1.),
        float2(0., 0.),
        float2(1., 1.),
        float2(0., 1.),
    };
    float2 position = positions[vertex_index];
    VertexOutput output;
    output.position = float4(position - float2(0.5, 0.5), 0.0, 1.0);
    output.uv       = float2(position.x, 1.0 - position.y);
    return output;
}

[shader("fragment")]
float4 main(VertexOutput input) {
    return float4(
        0.1 + 0.9  * input.uv.x,
        0.1 + 0.45 * (input.uv.x + input.uv.y),
        0.1 + 0.9  * input.uv.y,
        1.0);
}
