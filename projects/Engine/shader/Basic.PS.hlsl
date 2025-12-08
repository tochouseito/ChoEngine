struct PSInput
{
    float4 pos : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

float4 PSMain(PSInput input) : SV_TARGET
{
    float3 N = normalize(input.normal);
    float3 L = normalize(float3(0.3f, 0.7f, 0.2f));
    float d = saturate(dot(N, L));
    return float4(d.xxx, 1.0f);
}
