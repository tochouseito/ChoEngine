#include "../header/UIStandard.hlsli"

StructuredBuffer<World> gWorlds : register(t0);
StructuredBuffer<uint> gUIids : register(t1);


struct VSInput
{
    float4 position : POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR;
    uint vertexID : VERTEXID;
};
VSOutput main(VSInput input,uint instanceID : SV_InstanceID)
{
    VSOutput output;
    uint index = gUIids[instanceID];
    float4 position;
    float2 texcoord;
    switch (input.vertexID) {
        case 0:// 左上
            position = float4(gWorlds[index].left, gWorlds[index].bottom, 0.0f, 1.0f);
            texcoord = float2(gWorlds[index].tex_left, gWorlds[index].tex_bottom);
            break;
        case 1:// 右上
            position = float4(gWorlds[index].left, gWorlds[index].top, 0.0f, 1.0f);
            texcoord = float2(gWorlds[index].tex_left, gWorlds[index].tex_top);
            break;
        case 2: // 左下
            position = float4(gWorlds[index].right, gWorlds[index].bottom, 0.0f, 1.0f);
            texcoord = float2(gWorlds[index].tex_right, gWorlds[index].tex_bottom);
            break;
        case 3: // 右下
            position = float4(gWorlds[index].right, gWorlds[index].top, 0.0f, 1.0f);
            texcoord = float2(gWorlds[index].tex_right, gWorlds[index].tex_top);
            break;
        default:
            position = float4(0.0f, 0.0f, 0.0f, 1.0f);
            break;
    }
    output.position = mul(position, gWorlds[index].World);
    output.texcoord = texcoord;
    output.materialID = gWorlds[index].materialID;
    
    return output;
}