#include "../header/ScreenCopy.hlsli"
VSOutput main(uint vertexId : SV_VertexID)
{
    VSOutput output;
    output.position = kPositions[vertexId];
    output.texcoord = kTexcoords[vertexId];
    return output;
}