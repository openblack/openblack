// L3D vertex layout, including two packed signed 16-bit bone indices.
struct Vertex
{
    float3 position;
    float2 uv;
    float3 normal;
    uint boneIndices;
}

struct VOut
{
    float4 position : SV_Position;
    float4 worldPosition;
    float2 uv;
    float3 normal;
}

struct ObjectCamera
{
    float4x4 viewProjection;
    float4 islandExtent;
    uint4 offsets; // instance offset, bone offset, bone count, morph with terrain
}

[[vk::binding(0, 0)]] Sampler2D heightMap;
[[vk::binding(1, 0)]] StructuredBuffer<float4x4> matrices;
[[vk::binding(0, 1)]] ConstantBuffer<ObjectCamera> camera;
[[vk::binding(0, 2)]] Sampler2D diffuseTexture;
[[vk::binding(0, 3)]] ConstantBuffer<float4> skyAlphaThreshold;

[shader("vertex")]
VOut vertexMain(Vertex input, uint instanceID : SV_InstanceID)
{
    uint boneIndex = uint(max(0, int(input.boneIndices << 16) >> 16));
    boneIndex = min(boneIndex, camera.offsets.z - 1);
    float4x4 model = matrices[camera.offsets.x + instanceID];
    float4 worldPosition = mul(model, mul(matrices[camera.offsets.y + boneIndex], float4(input.position, 1.0)));
    if (camera.offsets.w != 0)
    {
        float2 uv = (worldPosition.xz - camera.islandExtent.xy) /
                    (camera.islandExtent.zw - camera.islandExtent.xy);
        float originalHeight = mul(model, float4(0, 0, 0, 1)).y;
        worldPosition.y += heightMap.SampleLevel(uv, 0).r * 170.85 - originalHeight;
    }
    VOut output;
    output.position = mul(camera.viewProjection, worldPosition);
    output.worldPosition = worldPosition;
    output.uv = input.uv;
    output.normal = input.normal;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VOut input) : SV_Target
{
    float brightness = skyAlphaThreshold.x / 2.0;
    float ambient = brightness * 0.25 + 0.25;
    float3 lightDirection = normalize(float4(-4000, 1300, -1435, 1) - input.worldPosition).xyz;
    float lighting = ambient + brightness * max(dot(input.normal, lightDirection), 0.0) * 0.75;
    float4 color = diffuseTexture.Sample(input.uv);
    if (color.a <= skyAlphaThreshold.y)
        discard;
    return float4(color.rgb * lighting, color.a);
}
