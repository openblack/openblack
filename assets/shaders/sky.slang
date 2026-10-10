struct Vertex
{
    float3 position;
    float2 uv;
    float3 normal;
    int16_t2 indices;
}

struct VOut
{
    float4 position : SV_Position;
    float2 uv;
float3 normal;
}

struct Camera
{
    float4x4 mvp;
}

struct Sky
{
    float timeOfDay;
    float alignment;
}

// In SDL-Gpu, the set 1 is for vertex shader uniforms
[[vk::binding(0, 1)]]
ConstantBuffer<Camera> camera;

// In SDL-Gpu, the set 3 is for fragment shader uniforms
[[vk::binding(0, 3)]]
ConstantBuffer<Sky> sky;
[[vk::binding(0, 2)]]
Sampler2DArray texture;

[shader("vertex")]
VOut vertexMain(Vertex input)
{
    VOut output;
    output.position = mul(camera.mvp, float4(input.position, 1.0));
    output.normal = input.normal;
    output.uv = input.uv;
    return output;
}

enum TimeOfDay
{
    Night = 0,
    Dusk = 1,
    Day = 2,
    _COUNT
}

enum Alignment
{
    Evil = 0,
    Neutral = 1,
    Good = 2,
    _COUNT
}

[shader("fragment")]
float4 fragmentMain(VOut input) : SV_Target
{
    float timeOfDay = clamp(sky.timeOfDay, float(TimeOfDay.Night), float(TimeOfDay.Day));
	float alignment = clamp(sky.alignment, float(Alignment.Evil), float(Alignment.Good));

	float alignT = fmod(alignment, 1.0f);
	float alignA = alignment - alignT;
	float alignB = min(alignA + 1.0f, float(Alignment.Good));

	float todT = fmod(timeOfDay, 1.0f);
	float todA = timeOfDay - todT;
	float todB = min(todA + 1.0f, float(TimeOfDay.Day));

	float indexAA = todA + float(TimeOfDay._COUNT) * alignA;
	float indexAB = todB + float(TimeOfDay._COUNT) * alignA;
	float indexBA = todA + float(TimeOfDay._COUNT) * alignB;
	float indexBB = todB + float(TimeOfDay._COUNT) * alignB;

	float4 colorAA = texture.Sample(float3(input.uv, indexAA));
	float4 colorAB = texture.Sample(float3(input.uv, indexAB));
	float4 colorBA = texture.Sample(float3(input.uv, indexBA));
	float4 colorBB = texture.Sample(float3(input.uv, indexBB));

	float4 colorAlignA = lerp(colorAA, colorBA, alignT);
	float4 colorAlignB = lerp(colorAB, colorBB, alignT);

    return lerp(colorAlignA, colorAlignB, todT);
}
