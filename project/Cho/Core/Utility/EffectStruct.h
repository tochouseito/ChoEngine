#pragma once
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Matrix4.h"
#include "Core/Utility/Color.h"
#include <cstdint>
#include <vector>
#include <string>
#include <variant>

struct RandValue
{
    float median;       // 中央値
    float amplitude;    // 振幅
};

struct Randfloat3
{
    RandValue x;
    RandValue y;
    RandValue z;
};

// イージングタイプ
enum class EFFECT_EASING_TYPE
{
    LEFTANDRIGHT = 0,
	LINEAR,
	EASE_IN_QUADRATIC,
	EASE_OUT_QUADRATIC,
	EASE_IN_OUT_QUADRATIC,
	EASE_IN_CUBIC,
	EASE_OUT_CUBIC,
	EASE_IN_OUT_CUBIC,
	EASE_IN_QUARTIC,
	EASE_OUT_QUARTIC,
	EASE_IN_OUT_QUARTIC,
	EASE_IN_QUINTIC,
	EASE_OUT_QUINTIC,
	EASE_IN_OUT_QUINTIC,
	EASE_IN_BACK,
	EASE_OUT_BACK,
	EASE_IN_OUT_BACK,
	EASE_IN_BOUNCE,
	EASE_OUT_BOUNCE,
	EASE_IN_OUT_BOUNCE,
	EASING_TYPE_COUNT
};

// イージングの速度タイプ
enum class EFFECT_SPEED_TYPE
{
	CONSTANT = 0,
	SLOW0,
	FAST0,
	SLOW1,
	FAST1,
	SLOW2,
	FAST2,
	EASING_SPEED_TYPE_COUNT
};

struct EasingValue
{
    Randfloat3 startPoint;     // 始点
    Randfloat3 endPoint;       // 終点
    uint32_t easingType;            // イージングタイプ
    uint32_t startSpeedType;        // 始点速度タイプ
    uint32_t endSpeedType;          // 終点速度タイプ
    uint32_t isMedianPoint;         // 中間点有効
    Randfloat3 medianPoint;    // 中間点
};

struct PVA
{
    Randfloat3 value = {};          // 値
    Randfloat3 velocity = {};       // 速度
    Randfloat3 acceleration = {};   // 加速度
};

enum class EFFECT_SRT_TYPE
{
	SRT_TYPE_STANDARD = 0,
	SRT_TYPE_PVA,
	SRT_TYPE_EASING,
};

struct EffectSRT
{
    uint32_t type = 0;          // タイプ
    float3 value;       // 値
    PVA pva;            // 値、速度、加速度
    EasingValue easing; // イージング値
};

enum class INFLUNCE_TYPE
{
	NONE = 0,       // 影響なし
	ALWAYS,   // 常に影響する
};

struct EffectCommon
{
	uint32_t emitCountMax = 1;           // 最大生成数
    uint32_t emitCount = 1;                 // 同時生成数
    //uint32_t emitCounter;          // 生成カウンタ(CPU側では使わない)
    uint32_t isUnlimited = 0;                 // 無限生成フラグ
    uint32_t PosInfluenceType = 0;          // 位置影響タイプ
    uint32_t RotInfluenceType = 0;          // 回転影響タイプ
    uint32_t SclInfluenceType = 0;          // スケール影響タイプ
    uint32_t deleteLifetime = 0;            // 寿命で削除
    uint32_t deleteParentDeleted = 0;       // 親削除で削除
    uint32_t deleteAllChildrenDeleted = 0;  // 全ての子削除で削除
    RandValue lifeTime = {};             // 生存時間
    RandValue emitInterval = {};             // 生成時間
    RandValue emitStartTime = {};        // 生成開始時間
};

// FadeOutタイプ
enum class FADEOUT_TYPE
{
	NONE = 0,   // フェードアウトしない
	ONLIFETIME, // 生存時間内でフェードアウト
	DELETED,    // 削除後にフェードアウト
	FADEOUT_TYPE_COUNT
};

// UVタイプ
enum class UV_TYPE
{
	STANDARD = 0,   // 標準UV
	CONSTANT,       // 固定UV
	ANIMATION,      // アニメーションUV
	SCROLL,         // スクロールUV
	UV_TYPE_COUNT
};

struct UVConstantValue
{
    float2 startPoint;  // 開始UV
    float2 scale;       // スケール
};

struct UVAnimationParameter
{
    float2 startPoint;      // 開始UV
    float2 scale;           // スケール
    uint32_t oneTime;           // 1回分の時間
    uint32_t widthSheetCount;   // 横の枚数
    uint32_t heightSheetCount;  // 縦の枚数
    uint32_t isLoop;            // ループフラグ
    RandValue startSheet;   // 開始シート
    uint32_t interpolation;     // 補間フラグ
};

struct EffectDrawCommon
{
    uint32_t materialType = 0;                      // マテリアルタイプ(現在は通常のみ)
	uint32_t textureID = 0;                          // テクスチャインデックス
    float emissive;                             // 発光値
    uint32_t isFadeIn;                              // フェードイン(0:しない 1:する)
    uint32_t isFadeOutType;                         // フェードアウトタイプ
    UVConstantValue constantValue;              // UV定数値
    UVAnimationParameter animationParameter;    // UVアニメーションパラメータ
};

enum class EFFECT_MESH_TYPE
{
	NONE = 0,       // メッシュタイプなし
	SPRITE,         // スプライト
	RIBBON,         // リボン
	TRAIL,          // トレイル
	RING,           // リング
	MODEL,          // モデル
	CUBE,           // 立方体
	SPHERE,         // 球
};

enum class COLOR_TYPE
{
	CONSTANT,       // 固定色
	RANDOM,         // ランダム色
	EASING,         // イージング色
};

struct RandColor
{
    RandValue r; // 赤
    RandValue g; // 緑
    RandValue b; // 青
    RandValue a; // アルファ
};

struct EasingColor
{
    Color startMinColor;   // 始点最小色
    Color startMaxColor;   // 始点最大色
    Color endMinColor;     // 終点最小色
    Color endMaxColor;     // 終点最大色
    EFFECT_SPEED_TYPE startSpeedType;    // 始点速度タイプ
    EFFECT_SPEED_TYPE endSpeedType;      // 終点速度タイプ
};

enum class PLACEMENT_TYPE
{
	NONE = 0,       // 配置タイプなし
	BILLBOARD,      // ビルボード
	BILLBOARDY,     // Y軸ビルボード
	BILLBOARDXY,    // Z軸回転ビルボード
	CONSTANT,       // 固定
};

enum class VERTEX_COLOR_TYPE
{
    STANDARD = 0,       // 標準
    CONSTANT,           // 固定
};

enum class VERTEX_POSITION_TYPE
{
	STANDARD = 0,       // 標準
	CONSTANT,           // 固定
};

struct SpriteVertexColor
{
    Color leftBottom;  // 左下色
    Color rightBottom; // 右下色
    Color leftTop;     // 左上色
    Color rightTop;    // 右上色
};

struct SpriteVertexPosition
{
    float2 leftBottom = { -0.5f,-0.5f };  // 左下座標
    float2 rightBottom={ 0.5f,-0.5f }; // 右下座標
    float2 leftTop = { -0.5f,0.5f };     // 左上座標
    float2 rightTop = { 0.5f,0.5f };    // 右上座標
};

struct EffectSprite
{
    uint32_t colorType=0;                         // 色タイプ
    Color color;                           // 色
    RandColor randColor;                    // ランダム色
    EasingColor easingColor;                // イージング色
    uint32_t placement = 0;                         // 配置方法
    uint32_t vertexColorType = 0;                   // 頂点色タイプ
    SpriteVertexColor vertexColor;          // 頂点色
    uint32_t vertexPositionType = 0;                 // 頂点座標タイプ
    SpriteVertexPosition vertexPosition;    // 頂点座標
};

enum class RING_SHAPETYPE
{
	DONUT = 0,       // ドーナツ型
	CRESCENT,       // 半円型
};

struct EffectRing
{
    uint32_t shape;
    float startFade;
    float endFade;
    float startAngle;
    float endAngle;
    uint32_t placement;
    uint32_t VertexCount;
    float2 outerRing;
    float2 innerRing;
    float centerRatio;
    Color outerRingColor;
    Color centerColor;
    Color innerRingColor;
};

struct EffectDraw
{
	uint32_t meshType = 1;          // メッシュタイプ
	uint32_t meshDataIndex;     // メッシュデータインデックス
};

// ノード
struct EffectNode
{
    EffectCommon common;            // 共通情報
    EffectSRT position;             // 位置
    EffectSRT rotation;             // 回転
    EffectSRT scale;                // スケール
    EffectDrawCommon drawCommon;    // 描画共通情報
    EffectDraw draw;                // 描画情報
	uint32_t isRootParent;           // ルート親フラグ
    uint32_t parentIndex;           // 親インデックス
};

struct EffectNodeData
{
    EffectCommon common;            // 共通情報
    EffectSRT position;             // 位置
    EffectSRT rotation;             // 回転
    EffectSRT scale;                // スケール
    EffectDrawCommon drawCommon;    // 描画共通情報
    EffectDraw draw;                // 描画情報
    uint32_t isRootParent;           // ルート親フラグ
    uint32_t parentIndex;           // 親インデックス
	std::string name;           // ノード名
	std::wstring textureName; // テクスチャ名
    std::variant<std::monostate, EffectSprite> drawMesh;
    uint32_t nodeID;
};

struct TimeManager
{
    float elapsedTime = 0.0f;
    float duration = 120.0f;
    float deltaTime = 0.0f;
    float padding[1];
};

struct EffectRoot
{
    TimeManager time;    // 時間管理
    uint32_t globalSeed = 0;
	uint32_t nodeCount = 0;              // ノード数
	uint32_t nodeID[128];           // ノードID
};

struct EffectRootData
{
    TimeManager time;    // 時間管理
    uint32_t globalSeed;
    uint32_t nodeCount;              // ノード数
	std::vector<EffectNodeData> nodes; // ノードデータ
};

struct EffectParticlePVA
{
    float3 value;
	float3 velocity;
	float3 acceleration;
};
struct EffectParticle
{
    float3 wPos;
	float3 wRot;
	float3 wScl;
	EffectParticlePVA position;         // 位置
	EffectParticlePVA rotation;         // 回転
	EffectParticlePVA scale;            // スケール
	Color color;               // 色
	float lifeTime;             // 寿命
	float spawnTime;         // 現在時間
	uint32_t isAlive;            // 生存フラグ
    uint32_t rootID;
	uint32_t nodeID;
	uint32_t meshType;          // メッシュタイプ
    uint32_t meshID;
};
