struct Vertex
{
    float2 position;
}

struct VOut
{
    float4 position : SV_Position;
    float3 uvAndDistance;
    float4 reflectionPosition;
}

struct Camera
{
    float4x4 view;
    float4x4 viewProjection;
}

struct Water
{
    float skyType;
}

[[vk::binding(0, 1)]] ConstantBuffer<Camera> camera;
[[vk::binding(0, 3)]] ConstantBuffer<Water> water;
[[vk::binding(0, 2)]] Sampler2D diffuseTexture;
[[vk::binding(1, 2)]] Sampler2D alphaTexture;
[[vk::binding(2, 2)]] Sampler2D reflectionTexture;

[shader("vertex")]
VOut vertexMain(Vertex input)
{
    float4 worldPosition = float4(input.position.x, 0.0, input.position.y, 1.0);
    float4 clipPosition = mul(camera.viewProjection, worldPosition);
    VOut output;
    output.position = clipPosition;
    output.uvAndDistance = float3(worldPosition.xz / 500.0, mul(camera.view, worldPosition).z);
    // SDL uses upward NDC Y and downward texture Y on every GPU backend.
    output.reflectionPosition = float4((clipPosition.xy * float2(1.0, -1.0) + clipPosition.ww) * 0.5,
                                      clipPosition.zw);
    return output;
}

[shader("fragment")]
float4 fragmentMain(VOut input) : SV_Target
{
    float brightness = lerp(0.25, 1.0, saturate(water.skyType / 2.0));
    float3 diffuse = brightness * diffuseTexture.Sample(input.uvAndDistance.xy).rgb;
    float2 reflectionUv = input.reflectionPosition.xy / input.reflectionPosition.w;
    float3 reflection = reflectionTexture.Sample(reflectionUv).rgb;
    float reflectionAlpha = alphaTexture.Sample(input.uvAndDistance.xy).r;
    float horizonAlpha = saturate(3000.0 / max(abs(input.uvAndDistance.z), 0.001));
    return float4(lerp(reflection, diffuse, reflectionAlpha * horizonAlpha), 1.0);
}
