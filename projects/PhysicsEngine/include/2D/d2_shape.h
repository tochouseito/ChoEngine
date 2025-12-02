#ifndef D2_SHAPE_H
#define D2_SHAPE_H

#include <chomath.h>
#include <memory>

namespace physics
{
	namespace d2
	{
		class Id2Body;
		class Id2Polygon;
		class Id2World;
		class box2dWorld;

		struct Id2ShapeDef
		{
			void* userData = nullptr; // ユーザーデータ
			float density = 1.0f; // 密度
			float friction = 0.5f; // 摩擦係数
			float restitution = 0.0f; // 反発係数
			bool isSensor = false; // センサーかどうか
		};

		class Id2Shape
		{
		public:
			virtual ~Id2Shape() = default;
			virtual void CreatePolygonShape(Id2Body*, Id2ShapeDef*, Id2Polygon*) {}
			virtual void Destroy() {} // 形状を削除
			bool IsActive() const { return isActive; } // 有効フラグの取得

		protected:
			bool isActive = true; // 有効フラグ
		};

		class box2dShape : public Id2Shape
		{
			public:
			box2dShape();
			~box2dShape() override = default;
			void CreatePolygonShape(Id2Body* body, Id2ShapeDef* shapeDef, Id2Polygon* polygon) override;
			void Destroy() override;
		private:
			struct Impl; // 実装の詳細を隠蔽するための前方宣言
			std::unique_ptr<Impl> impl; // 実装のポインタ
			box2dWorld* pWorld = nullptr; // 所属するワールド
		};

		class choPhysicsShape : public Id2Shape
		{
			public:
			choPhysicsShape() = default;
			~choPhysicsShape() override = default;
			void CreatePolygonShape(Id2Body* body, Id2ShapeDef* shapeDef, Id2Polygon* polygon) override
			{
				body; shapeDef; polygon; // ここで実際の処理を実装する
			}
			void Destroy() override
			{
				// ChoPhysicsの形状を削除する処理を実装
			}
		};
	}
}

#endif // D2_SHAPE_H
