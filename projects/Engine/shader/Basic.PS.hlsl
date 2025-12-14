struct PSInput
{
    float4 pos : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

struct PSOut
{
    float4 color : SV_TARGET;
};

PSOut PSMain(PSInput input)
{
    PSOut output;

    output.color = float4(0.0f, 0.0f, 0.0f, 1.0f);
    
    return output;
}
