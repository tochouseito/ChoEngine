#pragma once
#include"Vector3.h"
struct Sphere {
	Theatria::Math::float3 center; // !< 中心点
	float radius;   // !< 半径
};
struct Line {
	Theatria::Math::float3 origin; // !<始点
	Theatria::Math::float3 diff;   // !<終点への差分ベクトル
};
struct Ray {
	Theatria::Math::float3 origin; // !<始点
	Theatria::Math::float3 diff;   // !<終点への差分ベクトル
};
struct Segment {
	Theatria::Math::float3 origin; // !<始点
	Theatria::Math::float3 diff;   // !<終点への差分ベクトル
};
struct Plane {
	Theatria::Math::float3 normal; //!< 法線
	float distance; //!< 距離
};
struct Triangle
{
	Theatria::Math::float3 vertices[3];//!< 頂点
};
struct AABB {
	Theatria::Math::float3 min; //!<最小点
	Theatria::Math::float3 max; //!<最大点
};
struct Vector2Int {
	int x;
	int y;
};
struct OBB {
	Theatria::Math::float3 center; //!<中心点
	Theatria::Math::float3 orientations[3]; //!<座標軸、正規化，直交必須
	Theatria::Math::float3 size; //!< 座標方向の長さの半分。中心から面までの距離
};
