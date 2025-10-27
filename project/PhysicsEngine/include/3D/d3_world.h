#pragma once

#include "d3_body.h"
#include <memory>
#include <vector>
#include <functional>
#include <set>
#include <unordered_set>
#include <xhash>

class btDynamicsWorld;

namespace physics::d3
{
	enum class d3Backend
	{
		bullet,  // Bullet Physicsを使用
		chophysics // ChoPhysicsを使用
	};

	class Id3Shape;

	using OnContactFunc = std::function<void(int, int)>;

	class Id3World
	{
		public:
		virtual ~Id3World() = default;
		// 物理ワールドのバックエンドを取得
		d3Backend GetBackend() const { return backend; }
		// 重力を取得、設定
		virtual Theatria::Math::float3 GetGravity() const = 0;
		virtual void SetGravity(const Theatria::Math::float3& gravity) = 0;
		// シュミレーションのステップ
		virtual void Step(const float& deltaTime) = 0;
		// ボディを作成
		virtual Id3Body* CreateBody(const Id3BodyDef& bodyDef) = 0;
		// ボディを削除
		virtual void DestroyBody(Id3Body* body) = 0;
		// ステップ後に呼び出される衝突イベント
		virtual void ProcessEvents() {}
		// イベントコールバック関数登録
		virtual void SetBeginContactCallback(OnContactFunc) {}
		virtual void SetEndContactCallback(OnContactFunc) {}
		virtual void SetStayContactCallback(OnContactFunc) {}
	protected:
		d3Backend backend = {}; // 物理エンジンのバックエンド
		std::vector<std::unique_ptr<Id3Body>> bodies; // ワールド内のボディリスト
	};

	class bulletWorld : public Id3World
	{
		friend class bulletBody;
	public:
		bulletWorld();
		~bulletWorld() override;
		// 重力の設定、取得
		Theatria::Math::float3 GetGravity() const override;
		void SetGravity(const Theatria::Math::float3& gravity) override;
		void Step(const float& deltaTime) override;
		Id3Body* CreateBody(const Id3BodyDef& bodyDef)override;
		void DestroyBody(Id3Body* body)override;
		void ProcessEvents() override;
		// イベントコールバック関数の登録
		void SetBeginContactCallback(OnContactFunc callback) override
		{
			beginContactCallback = std::move(callback);
		}
		void SetEndContactCallback(OnContactFunc callback) override
		{
			endContactCallback = std::move(callback);
		}
		void SetStayContactCallback(OnContactFunc callback) override
		{
			stayContactCallback = std::move(callback);
		}
		btDynamicsWorld* GetDynamicsWorld() const; // Bulletの物理ワールドを取得
	private:
		struct Impl;
		std::unique_ptr<Impl> impl;
		// コールバック関数
		OnContactFunc beginContactCallback; // 衝突開始時のコールバック
		OnContactFunc endContactCallback;   // 衝突終了時のコールバック
		OnContactFunc stayContactCallback;  // 衝突継続時のコールバック
	};

	class chophysicsWorld : public Id3World
	{
	public:
		chophysicsWorld() {}
		~chophysicsWorld() override {}
		Theatria::Math::float3 GetGravity() const override { return Theatria::Math::float3(0,0,0); }
		void SetGravity(const Theatria::Math::float3&) override {}
		void Step(const float&) override {}
		Id3Body* CreateBody(const Id3BodyDef&) override { return nullptr; } // ChoPhysicsの実装ではボディを作成しない
		void DestroyBody(Id3Body*) override {} // ChoPhysicsの実装ではボディを削除しない
	};

	Id3World* CreateWorld(d3Backend backend);
	void DestroyWorld(Id3World* world) noexcept;
}

