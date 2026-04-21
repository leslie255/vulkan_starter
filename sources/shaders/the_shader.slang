struct VertexOutput {
    float4 position : SV_POSITION;
    float2 uv;
    // float3 normal;
};

[shader("vertex")]
VertexOutput main(uint vertex_index : SV_VulkanVertexID) {
    VertexOutput output;
    output.uv = float2(
            float((vertex_index << 1u) & 2u),
            float(vertex_index & 2u));
    output.position = float4(
            output.uv * 2.0 - float2(1.0),
            0.0,
            1.0) * 0.5;
    output.uv.y = 1.0 - output.uv.y;
    return output;
}

[shader("fragment")]
float4 main(VertexOutput input) {
    return float4(
        0.5 + 0.5 * input.uv.x,
        1.0,
        0.5 + 0.5 * input.uv.y,
        1.0);
}
