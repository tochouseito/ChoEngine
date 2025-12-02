#pragma once
#include "Math.hlsli"

// ALLBillboard
float4x4 BillboardMatrix(const float4x4 cameraMatrix) {
    float4x4 result;
    
    float cosY = cos(PI);
    float sinY = sin(PI);
    
    float4x4 backToFrontMatrix = {
        cosY, 0.0f, -sinY, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        sinY, 0.0f, cosY, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f};
    
    result = mul(backToFrontMatrix, cameraMatrix);
    result[3][0] = 0.0f;
    result[3][1] = 0.0f;
    result[3][2] = 0.0f;
    
    return result;
}

float4x4 FixedYAxisBillboardMatrix(
    float3 particleRot, 
    float3 particlePos, 
    float3 cameraPos
) {
    float3 fwdH = normalize(particlePos - cameraPos);
    fwdH.y = 0.0f;
    fwdH = normalize(fwdH);

    const float3 upW = float3(0, 1, 0);
    float3 right = normalize(cross(upW, fwdH));
    float3 up = cross(fwdH, right);
    
    float3x3 baseAxis = float3x3(
        right, 
        up, 
        fwdH 
    );
    
    float4x4 rotX4 = RotateX(particleRot.x);
    float4x4 rotY4 = RotateY(particleRot.y);
    float4x4 rotZ4 = RotateZ(particleRot.z);
    
    float4x4 localRot4 = mul(rotZ4, mul(rotX4, rotY4));
    
    float3x3 localRot = Take3x3(localRot4);
    
    float3x3 worldRot = mul(baseAxis, localRot);
    
    return float4x4(
        float4(worldRot[0], 0.0f), 
        float4(worldRot[1], 0.0f), 
        float4(worldRot[2], 0.0f), 
        float4(0.0f, 0.0f, 0.0f, 1.0f)
    );
}