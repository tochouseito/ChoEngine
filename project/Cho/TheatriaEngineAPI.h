#pragma once
#include "Engine/Engine.h"
#include "APIExportsMacro.h"
#include  <chomath.h>
#include "Platform/InputManager/InputManager.h"
#ifdef USE_THEATRIAENGINE_SCRIPT
#include "GameCore/ScriptAPI/ScriptAPI.h"
#endif
#include <variant>
#include <optional>
using GameParameterVariant = std::variant<int, float, bool, float3>;
class GameObject;
class TheatriaEngine;
namespace theatria
{
	// エディタとゲーム実行ファイルしか許可しない
#ifdef ENGINECREATE_FUNCTION
	// Engineの生成
	THEATRIA_API Engine* CreateEngine(RuntimeMode mode);
	// Engineの破棄
	THEATRIA_API void DestroyEngine(Engine* engine);
	// ポインタを受け取る
	THEATRIA_API void SetEngine(Engine* engine);
	// Engineのポインタ
	static TheatriaEngine* g_Engine = nullptr;

#endif
}
namespace theatriaSystem
{
	// スクリプト用
#ifdef USE_THEATRIAENGINE_SCRIPT
	// Json保存
	THEATRIA_API bool SaveGameParameter(const std::wstring& filePath,
		const std::string& group,
		const std::string& item,
		const std::string& dataName,
		const GameParameterVariant& value);
	// Json読み込み
	THEATRIA_API bool LoadGameParameter(const std::wstring& filePath,
		const std::string& group,
		const std::string& item,
		const std::string& dataName,
		GameParameterVariant& outValue);
	// ゲームオブジェクト取得
	THEATRIA_API GameObject* FindGameObjectByName(const std::wstring& name);
	// ゲームオブジェクト複製
	THEATRIA_API GameObject* CloneGameObject(const GameObject* srcObj, float3 generatePosition);
	// ゲームオブジェクト削除
	THEATRIA_API void DestroyGameObject(GameObject* obj);
	// デルタタイム取得
	THEATRIA_API float DeltaTime();
	// 解像度取得
	THEATRIA_API uint32_t ScreenWidth();
	THEATRIA_API uint32_t ScreenHeight();
	// Marionnette取得
	THEATRIA_API Marionnette* GetMarionnettePtr(const std::wstring& name);
	template<theatriaSystem::MarionnetteInterface T>
	T* GetMarionnette(const std::wstring& name)
	{
		return static_cast<T*>(GetMarionnettePtr(name));
	}

	// csv読み込み
	THEATRIA_API std::vector<std::vector<std::string>> LoadCSV(const std::string& filePath);
	THEATRIA_API std::vector<std::vector<int>> LoadCSV_Int(const std::string& filePath);

	// SceneManager
	struct SceneManagerAPI
	{
		THEATRIA_API void LoadScene(const std::wstring& sceneName);
		THEATRIA_API void UnloadScene(const std::wstring& sceneName);
		THEATRIA_API void ChangeMainScene(const std::wstring& sceneName);
	};
	THEATRIA_API extern SceneManagerAPI sceneManager;

	// TestAPI
	struct AudioAPI
	{
		THEATRIA_API AudioAPI();
		THEATRIA_API void AddSource(const std::string& name);
		THEATRIA_API void Play(const std::string& name, bool isLoop);
		THEATRIA_API void Stop(const std::string& name);
		THEATRIA_API bool IsPlaying(const std::string& name);
		THEATRIA_API void SetVolume(const std::string& name, float volume);
	private:
		struct ImplAudioAPI;
		std::unique_ptr<ImplAudioAPI> implAudioAPI = nullptr;
	};
	THEATRIA_API extern AudioAPI testAudio;

	// InputManager
	namespace Input
	{
		// キーの押下をチェック
		THEATRIA_API bool PushKey(uint8_t keyNumber);
		// キーのトリガーをチェック
		THEATRIA_API bool TriggerKey(uint8_t keyNumber);
		// 全マウス情報取得
		THEATRIA_API const DIMOUSESTATE2& GetAllMouse();
		// マウス移動量を取得
		THEATRIA_API MouseMove GetMouseMove();
		// マウスの押下をチェック
		THEATRIA_API bool IsPressMouse(int32_t mouseNumber);
		// マウスのトリガーをチェック。押した瞬間だけtrueになる
		THEATRIA_API bool IsTriggerMouse(int32_t buttonNumber);
		// マウスの位置を取得する（ウィンドウ座標系）
		THEATRIA_API const float2& GetMouseWindowPosition();
		// マウスの位置を取得する（ウィンドウ座標系）
		THEATRIA_API float2 GetMouseScreenPosition();
		// 現在のジョイスティック状態を取得する
		THEATRIA_API bool GetJoystickState(int32_t stickNo, XINPUT_STATE& out);
		// 前回のジョイスティック状態を取得する
		THEATRIA_API bool GetJoystickStatePrevious(int32_t stickNo, XINPUT_STATE& out);
		// デッドゾーンを設定する
		THEATRIA_API void SetJoystickDeadZone(int32_t stickNo, int32_t deadZoneL, int32_t deadZoneR);
		// デッドゾーンを取得する
		THEATRIA_API size_t GetNumberOfJoysticks();
		// 接続されているジョイスティック数を取得する
		THEATRIA_API bool IsTriggerPadButton(PadButton button, int32_t stickNo);
		// 接続されているジョイスティック数を取得する
		THEATRIA_API bool IsPressPadButton(PadButton button, int32_t stickNo);
		// 接続されているジョイスティック数を取得する
		THEATRIA_API float2 GetStickValue(LR padStick, int32_t stickNo);
		// 接続されているジョイスティック数を取得する
		THEATRIA_API float GetLRTrigger(LR LorR, int32_t stickNo);
	};

    namespace Log
    {

    }
#endif
}
