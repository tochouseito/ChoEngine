#pragma once

//get a scalar random value from a 3d value
float rand3dTo1d(float3 value, float3 dotDir = float3(12.9898, 78.233, 37.719)) {
    //make value smaller to avoid artefacts
    float3 smallValue = sin(value);
    //get scalar value from 3d vector
    float random = dot(smallValue, dotDir);
    //make value more random by making it bigger and then taking teh factional part
    random = frac(sin(random) * 143758.5453);
    random = random * 2.0 - 1.0;
    return random;
}
//get a 3d random value from a 3d value
float3 rand3dTo3d(float3 value) {
    return float3(
        rand3dTo1d(value, float3(12.989, 78.233, 37.719)),
        rand3dTo1d(value, float3(39.346, 11.135, 83.155)),
        rand3dTo1d(value, float3(73.156, 52.235, 09.151))
    );
}

float GenerateRandom(float seed) {
    return frac(sin(seed) * 43758.5453123);
}

float GenerateRandomInRange(float seed, float median, float amplitude) {
    float randomValue = GenerateRandom(seed);
    
    return median + amplitude * (randomValue * 2.0 - 1.0);
}

class RandomGenerator {
    float3 seed;
    float3 Generate3d() {
        seed = rand3dTo3d(seed);
        return seed;
    }
    float Generate1d() {
        float result = rand3dTo1d(seed);
        seed.x = result;
        return result;
    }
    float GenerateInRange(float min, float max) {
        float r = Generate1d();
        float normalized = (r + 1.0) * 0.5;
        return lerp(min, max, normalized);
    }
};
