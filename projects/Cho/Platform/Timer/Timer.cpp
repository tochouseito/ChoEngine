#include "pch.h"
#include "Timer.h"

Timer::Timer()
{
	Initialize();
}

Timer::~Timer()
{
}

void Timer::Initialize()
{
	/*現在時間を記録*/
	reference = std::chrono::steady_clock::now();
}

void Timer::Update()
{
	// フレームレートピッタリの時間
	std::chrono::microseconds kMinTime(static_cast<uint64_t>(1000000.0f / maxFrameRate));

	//// 1/60秒よりわずかに短い時間
	std::chrono::microseconds kMinCheckTime(uint64_t(1000000.0f / maxFrameRate + 4.0f));

	// 現在時間を取得する
	auto now = std::chrono::steady_clock::now();
	// 前回記録からの経過時間を取得する
	auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - reference);

	// 1/60秒 (よりわずかに短い時間) 経っていない場合
	if (elapsed < kMinCheckTime) {
		// 1/60秒経過するまで微小なスリープを繰り返す
		auto wait_until = reference + kMinTime;
		while (std::chrono::steady_clock::now() < wait_until) {
			std::this_thread::yield(); // CPUに他のスレッドの実行を許可
		}
	}

	// 現在の時間を記録する
	reference = std::chrono::steady_clock::now();

	// スリープ後の正確な経過時間を計算
	elapsed = std::chrono::duration_cast<std::chrono::microseconds>(reference - now);
}

void Timer::Start()
{
	startTime = std::chrono::high_resolution_clock::now();
}

void Timer::End()
{
	float frameTime = 0.0f;
	float fps = 0.0f;
	auto endTime = std::chrono::high_resolution_clock::now();
	std::chrono::duration<float> elapsed = endTime - startTime;
	frameTime = elapsed.count();

	if (frameTime > 0.0f) {
		fps = 1.0f / frameTime;
	}
	nowFPS = fps;
	deltaTime = frameTime;
}
