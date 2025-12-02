#pragma once
// Math

static const float PI = 3.14159265359;

float4x4 RotateX(float angle) {
    float s = sin(angle);
    float c = cos(angle);

    return float4x4(
        1, 0, 0, 0,
        0, c, s, 0,
        0, -s, c, 0,
        0, 0, 0, 1
    );
}

float4x4 RotateY(float angle) {
    float s = sin(angle);
    float c = cos(angle);

    return float4x4(
         c, 0, -s, 0,
         0, 1, 0, 0,
         s, 0, c, 0,
         0, 0, 0, 1
    );
}

float4x4 RotateZ(float angle) {
    float s = sin(angle);
    float c = cos(angle);

    return float4x4(
        c, s, 0, 0,
       -s, c, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    );
}

float4x4 RotateXYZ(float3 angles) {
    return mul(RotateZ(angles.z), mul(RotateY(angles.y), RotateX(angles.x)));
}

float4x4 ScaleMatrix(float3 scale) {
    return float4x4(
        scale.x, 0, 0, 0,
        0, scale.y, 0, 0,
        0, 0, scale.z, 0,
        0, 0, 0, 1
    );
}

float4x4 TranslateMatrix(float3 translate) {
    return float4x4(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        translate.x, translate.y, translate.z, 1
    );
}

float DegreeToRadian(const float degree)
{
    return degree * (PI / 180.0f);
}

float3 DegreesToRadians(const float3 degree)
{
    float3 result;
    result.x = DegreeToRadian(degree.x);
    result.y = DegreeToRadian(degree.y);
    result.z = DegreeToRadian(degree.z);
    return result;
}

float3x3 Take3x3(float4x4 m) {
    return float3x3(
        m[0].xyz,
        m[1].xyz,
        m[2].xyz
    );
}