#include "../header/Demo.hlsli"
#include "../header/Material.hlsli"
#include "../header/Light.hlsli"
#include "../header/Environment.hlsli"

// ライト
ConstantBuffer<Lights> gLights : register(b2);
// 環境
ConstantBuffer<Environment> gEnvironment : register(b3);
// IntegrationTransformResource
StructuredBuffer<Transform> gITF : register(t0, space1);
// マテリアル
StructuredBuffer<Material> gIMaterial : register(t1,space1);
// テクスチャリソース(カラー)
Texture2D<float4> gTextures[] : register(t2, space1);
// キューブテクスチャ
TextureCube<float4> gCubeTextures : register(t3, space2);
// サンプラー
SamplerState gSampler : register(s0);

struct PixelShaderOutput {
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VSOut input) {
    PixelShaderOutput output;
    
    // 最終出力
    float4 finalColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
    // 環境光
    float4 ambientColor = gEnvironment.ambientColor;
    // 頂点カラー
    float4 vertexColor = input.color;
    // マテリアル
    Material material = gIMaterial[input.materialID];
    //if (input.materialID == 0)
    //{
    //    // ID0なら頂点カラーを使用
    //    material.color.rgb = vertexColor.rgb;
    //}
    // テクスチャカラー
    float4 textureColor = float4(1.0f, 1.0f, 1.0f, 1.0f);
    // ライティング結果
    float3 lig = float3(0.0f, 0.0f, 0.0f);
    // カメラへの方向
    float3 toEye = normalize(input.cameraPosition - input.position.xyz);
    // 法線
    float3 normal = normalize(input.normal);
    // alphaが0ならdiscard
    if(material.color.a <= 0.0f) {
        discard;
    }
    // テクスチャが有効ならテクスチャカラーを取得
    if (material.enableTexture != 0)
    {
        float2 texcoord = input.texcoord;
        if (material.uvFlipY)
        {
            texcoord.y = 1.0f - texcoord.y;
        }
        // テクスチャ
        float4 transformedUV = mul(float4(texcoord, 0.0f, 1.0f), gIMaterial[input.materialID].matUV);
        textureColor = gTextures[material.textureID].Sample(gSampler, transformedUV.xy);
        // alphaが0ならdiscard
        if(textureColor.a <= 0.0f) {
            discard;
        }
    }
    // ライティングが有効ならライティングを計算
    if (material.enableLighting != 0)
    {
        // ライトのタイプごとにライティング
        for (int i = 0; i < MAX_LIGHTS; i++)
        {
            Light light = gLights.lights[i];
            // ライトが無効ならスキップ
            if (light.active == 0)
            {
                continue;
            }
            switch (light.type)
            {
                case LIGHT_TYPE_DIRECTIONAL:{// 平行光源
                        // ライトの位置を取得
                        float3 position =
                        {
                            gITF[light.transformMapID].matWorld[3][0],
                            gITF[light.transformMapID].matWorld[3][1],
                            gITF[light.transformMapID].matWorld[3][2]
                        };
                
                        float3 lightDir = -normalize(light.direction);
                        // 鏡面反射の強度を求める
                        float3 halfVector = normalize(lightDir + toEye);
                        float NDotH = saturate(dot(normal, halfVector)); // 鏡面反射強度
                        float specularPow = pow(NDotH, material.shininess);
                        // half lambert
                        float NdotL = dot(normal, lightDir);
                        // half lambertの計算
                        float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
                        // 拡散反射
                        float3 diffuseDirLight = light.color.rgb * cos * light.intensity;
                        // 鏡面反射
                        float3 specularDirLight = light.color.rgb * light.intensity * specularPow * float3(1.0f, 1.0f, 1.0f);
                        // 結果を加算
                        lig += diffuseDirLight + specularDirLight;
                        break;
                    }
                case LIGHT_TYPE_POINT:{// 点光源
                        // ライトの位置を取得
                        float3 position =
                        {
                            gITF[light.transformMapID].matWorld[3][0],
                        gITF[light.transformMapID].matWorld[3][1],
                        gITF[light.transformMapID].matWorld[3][2]
                        };
                        break;
                    }
                case LIGHT_TYPE_SPOT:{// スポットライト
                        // ライトの位置を取得
                        float3 position =
                        {
                            gITF[light.transformMapID].matWorld[3][0],
                        gITF[light.transformMapID].matWorld[3][1],
                        gITF[light.transformMapID].matWorld[3][2]
                        };
                        break;
                    }
            }
        }
        // ライティング結果と環境光を加算
        lig.rgb += ambientColor.rgb;
        // 合計
        finalColor.rgb = material.color.rgb * textureColor.rgb * lig.rgb;
        if (material.cubeTextureID != 0)
        {
            // 環境マップが有効なら環境マップを加算
            float3 reflectedVector = reflect(input.cameraPosition, normalize(input.normal));
            float4 environmentColor = gCubeTextures.Sample(gSampler, input.normal);
            finalColor.rgb += environmentColor.rgb;
        }
        finalColor.a = material.color.a * textureColor.a;
    }
    else {
        // 合計
        finalColor.rgb = material.color.rgb * textureColor.rgb;
        finalColor.a = material.color.a * textureColor.a;
    }
    // 最終出力
    output.color = finalColor;

    return output;
}
