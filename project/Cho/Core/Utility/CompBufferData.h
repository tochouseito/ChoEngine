#pragma once
#include "ChoMath.h"
#include "Core/Utility/Color.h"
#include "EffectStruct.h"
// 定数バッファ用データ構造体
// Transform
struct BUFFER_DATA_TF final
{
	// ローカル → ワールド変換行列
	float4x4 matWorld;		// 64バイト
    float4x4 worldInverse;	// 64バイト
	// モデルのRootMatrix
    float4x4 rootMatrix;		// 64バイト
	uint32_t materialID;	// 4バイト
	uint32_t isAnimated;	// 4バイト
	uint32_t boneOffsetStartIndex; // 4バイト
	float padding[1];	// 4バイト
};
// ViewProjection
struct BUFFER_DATA_VIEWPROJECTION final
{
    float4x4 view;				// 64バイト
    float4x4 projection;			// 64バイト
    float4x4 projectionInverse;	// 64バイト
    float4x4 matWorld;			// 64バイト
    float4x4 matBillboard;		// 64バイト
	float3 cameraPosition;		// 12バイト
	float padding1;				// 4バイト
};
// Line
struct BUFFER_DATA_LINE final
{
	float3 position;	// 12バイト
	Color color;		// 16バイト
};
// Material
struct BUFFER_DATA_MATERIAL final
{
	Color color;		// 16バイト
	float4x4 matUV;		// 64バイト
	float shininess;	// 4バイト
	uint32_t enableLighting;	// 4バイト
	uint32_t enableTexture;	// 4バイト
	uint32_t textureId;		// 4バイト
	uint32_t uvFlipY;
	uint32_t cubeTextureId;
	float cubeUVScale;	// 4バイト
};
// Emitter
struct BUFFER_DATA_EMITTER final
{
	RandValue lifeTime;
	PVA position;             // 位置
	PVA rotation;             // 回転
	PVA scale;                // スケール
	float frequency;	// 4バイト
	float frequencyTime;// 4バイト
	uint32_t emit;
	uint32_t emitCount;
	uint32_t isFadeOut;
	uint32_t isBillboard;	// 4バイト
	uint32_t materialID;	// 4バイト
};
// Particle
// PerFrame
struct BUFFER_DATA_PARTICLE_PERFRAME final
{
	float time;
	float deltaTime;
	float padding[2];
};
struct PVAfloat3Data// 位置,角度,拡縮
{
	float3 first = float3(0.0f, 0.0f, 0.0f);	// 最大,中央値
	float3 second = float3(0.0f, 0.0f, 0.0f);	// 最小,広がり幅
	// Random値形式
	uint32_t isMedian;							// ランダム値の設定方法。最大・最小、中央値・広がり幅の2種類
};

struct PVAData // Position,Velocity,Acceleration
{
	PVAfloat3Data value;			// 位置
	PVAfloat3Data velocity;		// 速度
	PVAfloat3Data acceleration;// 加速度
};
struct ParticlePositionData
{
	uint32_t type;		// 位置のタイプ
	float3 value;		// 位置
	PVAData pva;		// PVA
};

struct ParticleRotationData
{
	uint32_t type;		// 位置のタイプ
	float3 value;		// 位置
	PVAData pva;		// PVA
};

struct ParticleScaleData
{
	uint32_t type;		// 位置のタイプ
	float3 value;		// 位置
	PVAData pva;		// PVA
};
struct ParticlePVAData
{
	float3 value;
	float3 velocity;
	float3 acceleration;
};
struct BUFFER_DATA_PARTICLE final
{
	ParticlePVAData position;	// 36バイト
	ParticlePVAData rotation;	// 36バイト
	ParticlePVAData scale;		// 36バイト
	Color color;				// 16バイト
	float lifeTime;				// 4バイト
	float currentTime;			// 4バイト
	uint32_t isFadeOut;		// 4バイト
	uint32_t isBillboard;
	uint32_t isAlive;				// 4バイト
	uint32_t materialID;	// 4バイト
};

struct BUFFER_DATA_UISPRITE final
{
	float4x4 matWorld;		// 64バイト
	float left;				// 4バイト
	float right;			// 4バイト
	float top;				// 4バイト
	float bottom;			// 4バイト
	float tex_left;			// 4バイト
	float tex_right;		// 4バイト
	float tex_top;			// 4バイト
	float tex_bottom;		// 4バイト
	uint32_t materialID;	// 4バイト
};

// 平行光源の数
static const uint32_t kDirLightNum = 10;
// 点光源の数
static const uint32_t kPointLightNum = 10;
// スポットライトの数
static const uint32_t kSpotLightNum = 10;

struct BUFFER_DATA_DIRECTIONALLIGHT
{
	float3 color;    //!<ライトの色
	float intensity;  //!<ライトの強さ
	float3 direction;//!<ライトの向き
	uint32_t active;  //!<ライトの有効無効
};


struct BUFFER_DATA_POINTLIGHT
{
	float3 color;    //!<ライトの色
	float intensity;  //!<ライトの強さ
	float3 position;//!<ライトの位置
	float radius;//!<ライトの届く最大距離
	float decay;//!<減衰率
	uint32_t active;  //!<ライトの有効無効
	float pad1[2];
};

struct BUFFER_DATA_SPOTLIGHT
{
	float3 color;    //!<ライトの色
	float intensity;  //!<ライトの強さ
	float3 direction;//!<ライトの向き
	float distance;//!<ライトの届く最大距離
	float3 position; //!<ライトの位置
	float decay;//!<減衰率
	float cosAngle;//!<スポットライトの余弦
	float cosFalloffStart;//Falloff開始の角度
	uint32_t active;  //!<ライトの有効無効
	float pad1;
};

struct BUFFER_DATA_PUNCTUALLIGHT
{
	// 環境光
	float3 ambientColor;
	float pad1;
	// 平行光源
	std::array<BUFFER_DATA_DIRECTIONALLIGHT, kDirLightNum> dirLights;
	// 点光源
	std::array<BUFFER_DATA_POINTLIGHT, kPointLightNum> pointLights;
	// スポットライト
	std::array<BUFFER_DATA_SPOTLIGHT, kSpotLightNum> spotLights;
};

static const uint32_t kMaxLight = 30;// ライトの最大数

struct LightData
{
	Color color;		// 色
	float3 direction;	// 向き
	float intensity;	// 強度
	float range;		// 適用距離
	float decay;		// 減衰率
	float spotAngle;	// スポットライトの角度
	float spotFalloffStart;
	uint32_t type;		// ライトの種類
	uint32_t active;	// ライトの有効無効
	uint32_t transformMapID;
	float pad[1];		// パディング
};

struct BUFFER_DATA_LIGHT final
{
	LightData lightData[kMaxLight];	// ライトデータ
};

struct BUFFER_DATA_ENVIRONMENT final
{
	Color ambientColor;	// 環境光の色
};