#ifndef D2_POLYGON_H
#define D2_POLYGON_H

#include <chomath.h>
#include <memory>

// box2d
struct b2Polygon;

namespace physics
{
	namespace d2
	{
		class Id2Polygon
		{
			//friend class box2dShape; // box2dShapeがId2Polygonの内部実装にアクセスできるようにする
			public:
			virtual ~Id2Polygon() = default;
			virtual void MakeBox(const float&, const float&) {};
			void SetSize(const float& w, const float& h) { width = w; height = h; }
			Theatria::Math::float2 GetSize() const { return Theatria::Math::float2(width, height); } // サイズを取得
		protected:
			float width = 0.0f; // 幅
			float height = 0.0f; // 高さ
		};

		class box2dPolygon : public Id2Polygon
		{
			friend class box2dShape; // box2dShapeがbox2dPolygonの内部実装にアクセスできるようにする
		public:
				box2dPolygon();
			~box2dPolygon() override = default;
			void MakeBox(const float& halfWidth, const float& halfHeight) override;
		private:
			const b2Polygon* GetPolygon() const; // Box2Dのポリゴンを取得
			struct Impl; // 実装の詳細を隠蔽するための前方宣言
			std::unique_ptr<Impl> impl; // 実装のポインタ
		};

		class choPhysicsPolygon : public Id2Polygon
		{
			public:
				choPhysicsPolygon();
			~choPhysicsPolygon() override = default;
			void MakeBox(const float& halfWidth, const float& halfHeight) override
			{
				halfHeight; halfWidth;
			}
		private:
			struct Impl; // 実装の詳細を隠蔽するための前方宣言
			std::unique_ptr<Impl> impl; // 実装のポインタ
		};
	}
}

#endif // D2_POLYGON_H