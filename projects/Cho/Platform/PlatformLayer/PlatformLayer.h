#pragma once

/*--------------------------------------------
プラットフォーム独立層
--------------------------------------------*/
#include "Platform/Timer/Timer.h"
#include "Platform/InputManager/InputManager.h"
#include "Platform/FileSystem/FileSystem.h"

//C++
#include <memory>

class PlatformLayer
{
public:// メンバ関数
	// コンストラクタ
	PlatformLayer();
	// デストラクタ
	~PlatformLayer();
	// 初期化
	void Initialize();
	// 終了処理
	void Finalize();
	// 更新
	void Update();
	// 記録開始
	void StartFrame();
	// 記録終了
	void EndFrame();

	InputManager* GetInputManager() { return m_InputManager.get(); }
private:// メンバ変数
	std::unique_ptr<Timer> m_Timer = nullptr;
	std::unique_ptr<InputManager> m_InputManager = nullptr;
};

