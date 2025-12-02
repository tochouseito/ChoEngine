#pragma once
/*--------------------------------------------
CoreSystemクラス
--------------------------------------------*/

#include "Core/ThreadManager/ThreadManager.h"

// C++
#include <memory>

class CoreSystem
{
public:// メンバ関数
	// コンストラクタ
	CoreSystem();

	// デストラクタ
	~CoreSystem();

	// 初期化
	void Initialize();

	// 終了処理
	void Finalize();

    // ThreadManagerの取得
    ThreadManager* GetThreadManager() { return threadManager.get(); }

private:
    // ThreadManager
    std::unique_ptr<ThreadManager> threadManager = nullptr;
};

