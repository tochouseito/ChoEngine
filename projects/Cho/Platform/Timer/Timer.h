#pragma once
/*--------------------------------------------
タイマークラス
* --------------------------------------------*/

#include <iostream>
#include <chrono>
#include <thread>

class Timer
{
public:// メンバ関数

	// コンストラクタ
	Timer();

	// デストラクタ
	~Timer();

	// 初期化
	void Initialize();

	// 更新
	void Update();

	// 記録開始
	void Start();

	// 記録終了
	void End();

	static float GetDeltaTime() { return deltaTime; }// デルタタイム取得

private:// メンバ変数
	std::chrono::steady_clock::time_point reference = {};// 記録時間FPS固定用
	std::chrono::time_point<std::chrono::high_resolution_clock> startTime = {};

	float maxFrameRate = 60.0f;// 最大フレームレート
	float nowFPS = maxFrameRate;// 現在のフレームレート
	static inline float deltaTime;// = 1.0f / nowFPS;// デルタタイム
};

inline float DeltaTime() { return Timer::GetDeltaTime(); }// デルタタイム取得
