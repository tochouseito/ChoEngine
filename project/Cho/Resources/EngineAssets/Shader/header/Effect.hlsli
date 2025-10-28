#pragma once

struct RandValue {
    float median;       
    float amplitude;    
};

struct Randfloat3 {
    RandValue x;
    RandValue y;
    RandValue z;
};

static const uint EASING_TYPE_COUNT = 20;
static const uint EASING_TYPE_LEFTANDRIGHT = 0;
static const uint EASING_TYPE_LINEAR = 1;
static const uint EASING_TYPE_EASEINQUADRATIC = 2;
static const uint EASING_TYPE_EASEOUTQUADRATIC = 3;
static const uint EASING_TYPE_EASEINOUTQUADRATIC = 4;
static const uint EASING_TYPE_EASEINCUBIC = 5;
static const uint EASING_TYPE_EASEOUTCUBIC = 6;
static const uint EASING_TYPE_EASEINOUTCUBIC = 7;
static const uint EASING_TYPE_EASEINQUARTIC = 8;
static const uint EASING_TYPE_EASEOUTQUARTIC = 9;
static const uint EASING_TYPE_EASEINOUTQUARTIC = 10;
static const uint EASING_TYPE_EASEINQUINTIC = 11;
static const uint EASING_TYPE_EASEOUTQUINTIC = 12;
static const uint EASING_TYPE_EASEINOUTQUINTIC = 13;
static const uint EASING_TYPE_EASEINBACK = 14;
static const uint EASING_TYPE_EASEOUTBACK = 15;
static const uint EASING_TYPE_EASEINOUTBACK = 16;
static const uint EASING_TYPE_EASEINBOUNCE = 17;
static const uint EASING_TYPE_EASEOUTBOUNCE = 18;
static const uint EASING_TYPE_EASEINOUTBOUNCE = 19;

static const uint EASING_SPEEDTYPE_CONSTANT = 0;
static const uint EASING_SPEEDTYPE_SLOW0 = 1;
static const uint EASING_SPEEDTYPE_FAST0 = 2;
static const uint EASING_SPEEDTYPE_SLOW1 = 3;
static const uint EASING_SPEEDTYPE_FAST1 = 4;
static const uint EASING_SPEEDTYPE_SLOW2 = 5;
static const uint EASING_SPEEDTYPE_FAST2 = 6;

struct EasingValue {
    Randfloat3 startPoint;     
    Randfloat3 endPoint;       
    uint easingType;            
    uint startSpeedType;       
    uint endSpeedType;         
    uint isMedianPoint;         
    Randfloat3 medianPoint;    
};

struct PVA {
    Randfloat3 value;          
    Randfloat3 velocity;      
    Randfloat3 acceleration;   
};

static const uint SRT_TYPE_STANDARD = 0;
static const uint SRT_TYPE_PVA = 1;
static const uint SRT_TYPE_EASING = 2; 

struct EffectSRT {
    uint type;         
    float3 value;       
    PVA pva;            
    EasingValue easing; 
};

struct EffectCommon {
    uint emitCountMax;    
    uint emitCount;
    //uint emitCounter;
    uint isUnlimit;                
    uint PosInfluenceType;        
    uint RotInfluenceType;        
    uint SclInfluenceType;          
    uint deleteLifetime;        
    uint deleteParentDeleted;     
    uint deleteAllChildrenDeleted;  
    RandValue lifeTime;           
    RandValue emitTime;           
    RandValue emitStartTime;      
};

static const uint FADEOUT_TYPE_NONE = 0;       
static const uint FADEOUT_TYPE_ONLIFETIME = 1; 
static const uint FADEOUT_TYPE_DELETED = 2;    

static const uint UV_TYPE_STANDERD = 0;     
static const uint UV_TYPE_CONSTANT = 1;     
static const uint UV_TYPE_ANIMATION = 2;    
static const uint UV_TYPE_SCROLL = 3;       

struct UVConstantValue {
    float2 startPoint;  
    float2 scale;       
};

struct UVAnimationParameter {
    float2 startPoint;      
    float2 scale;           
    uint oneTime;          
    uint widthSheetCount;  
    uint heightSheetCount; 
    uint isLoop;           
    RandValue startSheet;  
    uint interpolation;   
};

struct EffectDrawCommon {
    uint materialType;  
    uint textureID;
    float emissive;                             
    uint isFadeIn;                              
    uint isFadeOutType;                        
    UVConstantValue constantValue;             
    UVAnimationParameter animationParameter;   
};

static const uint MESH_TYPE_NONE = 0;      
static const uint MESH_TYPE_SPRITE = 1;    
static const uint MESH_TYPE_RIBBON = 2;    
static const uint MESH_TYPE_TRAIL = 3;      
static const uint MESH_TYPE_RING = 4;       
static const uint MESH_TYPE_MODEL = 5;      
static const uint MESH_TYPE_CUBE = 6;      
static const uint MESH_TYPE_SPHERE = 7;     

static const uint COLOR_TYPE_CONSTANT = 0;  
static const uint COLOR_TYPE_RANDOM = 1;    
static const uint COLOR_TYPE_EASING = 2;    

struct RandColor {
    float4 minColor;
    float4 maxColor;
};

struct EasingColor {
    float4 startMinColor;   
    float4 startMaxColor;   
    float4 endMinColor;     
    float4 endMaxColor;   
    uint startSpeedType;   
    uint endSpeedType;     
};

static const uint PLACEMENT_TYPE_BILLBOARD = 0;     
static const uint PLACEMENT_TYPE_BILLBOARDY = 1;   
static const uint PLACEMENT_TYPE_BILLBOARDXY = 2;   
static const uint PLACEMENT_TYPE_CONSTANT = 3;      

static const uint VERTEX_COLOR_TYPE_STANDARD = 0;
static const uint VERTEX_COLOR_TYPE_CONSTANT = 1; 

static const uint VERTEX_TYPE_STANDARD = 0; 
static const uint VERTEX_TYPE_CONSTANT = 1; 

struct SpriteVertexColor {
    float4 leftBottom;
    float4 rightBottom; 
    float4 leftTop;     
    float4 rightTop;    
};

struct SpriteVertexPosition {
    float2 leftBottom;  
    float2 rightBottom; 
    float2 leftTop;     
    float2 rightTop;    
};

struct EffectSprite {
    uint colorType;                         
    float4 color;                          
    RandColor randColor;                    
    EasingColor easingColor;                
    uint placement;                         
    uint VertexColorType;                   
    SpriteVertexColor vertexColor;          
    uint VertexPositionType;
    SpriteVertexPosition vertexPosition;   
};

struct EffectDraw {
    uint meshType;          
    uint meshDataIndex;    
};

struct EffectNode {
    EffectCommon common;            
    EffectSRT position;             
    EffectSRT rotation;             
    EffectSRT scale;                
    EffectDrawCommon drawCommon;   
    EffectDraw draw;               
    uint parentIndex;
};

struct TimeManager {
    float elapasedTime;
    float duration;
    float deltaTime;
    float padding;
};

static const uint kMaxRoot = 128;
static const uint kMaxNode = 1024;
static const uint kMaxMeshData = 1024;
static const uint kMaxParticles = 1024;

struct EffectRoot {
    TimeManager time; 
    uint globalSeed;
    uint nodeCount;
    uint2 padding;
    uint4 nodeIDs[32];
};

struct EffectParticlePVA {
    float3 value;
    float3 velocity;
    float3 acceleration;
};
struct EffectParticle {
    EffectParticlePVA position;
    EffectParticlePVA rotation;
    EffectParticlePVA scale;
    float4 color;
    float lifeTime;
    float currentTime;
    uint isAlive;
    uint rootID;
    uint nodeID;
    uint meshID;
};

struct VSInput {
    float4 position : POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float4 color : COLOR;
    uint vertexID : VERTEXID;
    
};
struct VSOutput {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float4 color : COLOR0;
    uint instanceId : SV_InstanceID;
};

uint FetchEffectNodeID(EffectRoot root,uint idx)
{
    uint row = idx >> 2; // idx / 4
    uint col = idx & 3; // idx % 4
    return root.nodeIDs[row][col];
}