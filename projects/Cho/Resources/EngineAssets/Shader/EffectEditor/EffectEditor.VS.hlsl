#include "../header/Effect.hlsli"
#include "../header/Billboard.hlsli"
#include "../header/ViewProjection.hlsli"

// CBV: ビュープロジェクションリソース
ConstantBuffer<ViewProjection> gVP : register(b0);
// CBV : EffectRoot
ConstantBuffer<EffectRoot> gEffectRoot : register(b1);
// SRV : EffectNode
StructuredBuffer<EffectNode> gEffectNode : register(t0);
// SRV : EffectMeshData
StructuredBuffer<EffectSprite> gEffectMesh : register(t1);

// UAV : EffectParticle 128x1024個
RWStructuredBuffer<EffectParticle> gEffectParticle : register(u0);

//// --- 3×3 だけ取り出すユーティリティ ------------------------------
//float3x3 Take3x3(float4x4 m)
//{
//    // 行優先なら 0,1,2 行目の xyz がそのまま回転部分
//    return float3x3(
//        m[0].xyz,
//        m[1].xyz,
//        m[2].xyz
//    );
//}

//// --- Fixed‑Y ビルボード ------------------------------------------
//float4x4 FixedYAxisBillboardMatrix(
//    float3 particleRot, // Euler XYZ (rad)
//    float3 particlePos, // パーティクル位置 (world)
//    float3 cameraPos // カメラ位置 (world)
//)
//{
//    //----------------------------------------------------------------
//    // ① ベース姿勢：カメラ方向を水平投影した軸
//    //----------------------------------------------------------------
//    float3 fwdH = normalize(particlePos - cameraPos);
//    fwdH.y = 0.0f;
//    fwdH = normalize(fwdH);

//    const float3 upW = float3(0, 1, 0);
//    float3 right = normalize(cross(upW, fwdH));
//    float3 up = cross(fwdH, right);

//    // row‑major 系：行ベクトルで格納する
//    float3x3 baseAxis = float3x3(
//        right, // 行0
//        up, // 行1
//        fwdH // 行2
//    );

//    //----------------------------------------------------------------
//    // ② ローカル回転：ユーザーの RotateX/Y/Z (float4x4) を流用
//    //----------------------------------------------------------------
//    float4x4 rotX4 = RotateX(particleRot.x);
//    float4x4 rotY4 = RotateY(particleRot.y);
//    float4x4 rotZ4 = RotateZ(particleRot.z);

//    // Effekseer 1.7～ は Y → X → Z（row‑vector 系なら Z*X*Y のコード）
//    float4x4 localRot4 = mul(rotZ4, mul(rotX4, rotY4)); // Z * X * Y

//    // 上左 3×3 部分だけ取得
//    float3x3 localRot = Take3x3(localRot4);

//    //----------------------------------------------------------------
//    // ③ 合成（ベース姿勢 → ローカル回転）
//    //----------------------------------------------------------------
//    // 行ベクトル系： baseAxis が先，localRot が後
//    float3x3 worldRot = mul(baseAxis, localRot);

//    //----------------------------------------------------------------
//    // ④ 4×4 に拡張して返す（位置は 0）
//    //----------------------------------------------------------------
//    return float4x4(
//        float4(worldRot[0], 0.0f), // 行0
//        float4(worldRot[1], 0.0f), // 行1
//        float4(worldRot[2], 0.0f), // 行2
//        float4(0.0f, 0.0f, 0.0f, 1.0f)
//    );
//}

VSOutput main(VSInput input, uint instanceId : SV_InstanceID,uint vertexId : SV_VertexID){
    VSOutput output;
    
    EffectParticle particle = gEffectParticle[instanceId];
    
    // worldViewProjection
    float4x4 WVP = mul(gVP.view, gVP.projection);
    // cameraMatrix
    float4x4 cameraMatrix = gVP.matWorld;
    // cameraPosition
    float3 cameraPosition = gVP.cameraPosition;
    // scaleMatrix
    float4x4 scaleMatrix = ScaleMatrix(particle.scale.value);
    // translateMatrix
    float4x4 translateMatrix = TranslateMatrix(particle.position.value);
    // rotationMatrix
    
    // billboardMatrix
    float4x4 billboardMatrix;
    switch (gEffectMesh[particle.meshID].placement)
    {
        case PLACEMENT_TYPE_BILLBOARD:{
                billboardMatrix = BillboardMatrix(cameraMatrix);
                break;
            }
        case PLACEMENT_TYPE_BILLBOARDY:{
                float3 radian = DegreesToRadians(particle.rotation.value);
                billboardMatrix = FixedYAxisBillboardMatrix(radian,particle.position.value,gVP.cameraPosition);
                break;
            }
        case PLACEMENT_TYPE_BILLBOARDXY:{
                float3 radian = DegreesToRadians(particle.rotation.value);
                //billboardMatrix = SphereBillboardMatrix(cameraMatrix, cameraPosition, particle.position.value);
                break;
            }
        case PLACEMENT_TYPE_CONSTANT:{
                float3 radian = DegreesToRadians(particle.rotation.value);
                billboardMatrix = RotateXYZ(radian);
                break;
            }
    }
    // worldMatrix
    float4x4 worldMatrix = mul(mul(scaleMatrix, billboardMatrix), translateMatrix);
    //worldMatrix[0] *= particle.scale.value.x;
    //worldMatrix[1] *= particle.scale.value.y;
    //worldMatrix[2] *= particle.scale.value.z;
    //worldMatrix[3].xyz = particle.position.value;
    
    // 頂点座標タイプで頂点座標を変更
    float4 vertexPosition = input.position;
    
    if (gEffectMesh[particle.meshID].VertexPositionType == VERTEX_TYPE_CONSTANT) {
        switch (input.vertexID) {
            case 0:
                vertexPosition.xy = gEffectMesh[particle.meshID].vertexPosition.rightTop;
                break;
            case 1:
                vertexPosition.xy = gEffectMesh[particle.meshID].vertexPosition.leftTop;
                break;
            case 2:
                vertexPosition.xy = gEffectMesh[particle.meshID].vertexPosition.rightBottom;
                break;
            case 3:
                vertexPosition.xy = gEffectMesh[particle.meshID].vertexPosition.leftBottom;
                break;
            case 4:
                vertexPosition.xy = gEffectMesh[particle.meshID].vertexPosition.leftBottom;
                break;
            case 5:
                vertexPosition.xy = gEffectMesh[particle.meshID].vertexPosition.rightBottom;
                break;
        }
    }
    
    output.position = mul(vertexPosition, mul(worldMatrix, WVP));
    output.texcoord = input.texcoord;
    output.normal = input.normal;
    output.color = particle.color;
    output.instanceId = instanceId;
    
    return output;
}