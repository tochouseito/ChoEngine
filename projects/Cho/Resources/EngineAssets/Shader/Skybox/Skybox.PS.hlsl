#include "../header/Skybox.hlsli"

TextureCube<float4> gTextureCube : register(t0);
SamplerState gSampler : register(s0);

struct PSPOutput {
    float4 color : SV_Target0;
};

// 回転行列（各軸）を作る
float3x3 RotationMatrixXYZ(float angleX, float angleY, float angleZ)
{
    float cx = cos(angleX), sx = sin(angleX);
    float cy = cos(angleY), sy = sin(angleY);
    float cz = cos(angleZ), sz = sin(angleZ);

    // X軸回転行列
    float3x3 rotX =
    {
        1, 0, 0,
        0, cx, -sx,
        0, sx, cx
    };

    // Y軸回転行列
    float3x3 rotY =
    {
        cy, 0, sy,
          0, 1, 0,
        -sy, 0, cy
    };

    // Z軸回転行列
    float3x3 rotZ =
    {
        cz, -sz, 0,
        sz, cz, 0,
         0, 0, 1
    };

    // 合成順序：Z → X → Y（任意に変更可能）
    return mul(rotY, mul(rotX, rotZ));
}

PSPOutput main(VSOutput input)
{
    PSPOutput output;
     // 各軸の回転角度（ラジアン）
    float angleX = radians(-90.0f); // X軸 -90度
    float angleY = radians(0.0f); // Y軸 0度
    float angleZ = radians(0.0f); // Z軸 0度

    float3x3 rot = RotationMatrixXYZ(angleX, angleY, angleZ);

    // 入力方向ベクトルに回転を適用
    float3 rotatedDir = mul(input.texCoord, rot);
    float4 textureColor = gTextureCube.Sample(gSampler, rotatedDir);
    output.color = textureColor;
    return output;
}