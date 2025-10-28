#pragma once
#include <chomath.h>
#include "Core/Utility/InputStruct.h"
#include <vector>
#include <wrl.h>
using namespace Microsoft::WRL;

class InputManager
{
public:
	InputManager()
	{
		Initialize();
	}
	~InputManager()
	{

	}
	void Initialize();
	void Update();
	// キーの押下をチェック
	bool PushKey(const uint8_t& keyNumber) const;
	// キーのトリガーをチェック
	bool TriggerKey(const uint8_t& keyNumber) const;
	// 全マウス情報取得
	const DIMOUSESTATE2& GetAllMouse() const;
	// マウスの押下をチェック
	bool IsPressMouse(const int32_t& mouseNumber) const;
	// マウスのトリガーをチェック。押した瞬間だけtrueになる
	bool IsTriggerMouse(const int32_t& buttonNumber) const;
	// マウス移動量を取得
	MouseMove GetMouseMove();
	// ホイールスクロール量を取得する
	int32_t GetWheel() const;
	// マウスの位置を取得する（ウィンドウ座標系）
	const float2& GetMouseWindowPosition() const;
	// マウスの位置を取得する（スクリーン座標系）
	float2 GetMouseScreenPosition() const;
	// 現在のジョイスティック状態を取得する
	bool GetJoystickState(const int32_t& stickNo, XINPUT_STATE& out) const;
	// 前回のジョイスティック状態を取得する
	bool GetJoystickStatePrevious(const int32_t& stickNo, XINPUT_STATE& out) const;
	// デッドゾーンを設定する
	void SetJoystickDeadZone(const int32_t& stickNo, const int32_t& deadZoneL, const int32_t& deadZoneR);
	// デッドゾーンを取得する
	int32_t GetLeftStickDeadZone() const { return m_LeftStickDeadZone; }
	int32_t GetRightStickDeadZone() const { return m_RightStickDeadZone; }

	// 接続されているジョイスティック数を取得する
	size_t GetNumberOfJoysticks();

	// パッドの押されているボタン、スティックの値を取得
	bool IsTriggerPadButton(const PadButton& button, int32_t stickNo = 0);
	bool IsPressPadButton(const PadButton& button, int32_t stickNo = 0);
	float2 GetStickValue(const LR& padStick, int32_t stickNo = 0);
	float GetLRTrigger(const LR& LorR, int32_t stickNo = 0);
	float2 CheckAndWarpMouse();
private:
	bool IsTriggerTrigger(const LR& LorR, int32_t stickNo = 0);
	float2 ApplyRadialDeadZone(int16_t rawX, int16_t rawY, int deadZone);
public:
	ComPtr<IDirectInput8> m_DirectInput = nullptr;
	ComPtr<IDirectInputDevice8> m_Keyboard = nullptr;
	ComPtr<IDirectInputDevice8> m_DIMouse = nullptr;
	std::vector<Joystick> m_Joysticks;
	BYTE m_Key[256] = {};
	BYTE m_PreKey[256] = {};
	DIMOUSESTATE2 m_Mouse = {};
	DIMOUSESTATE2 m_MousePre = {};
	float2 m_MousePosition;
	int32_t m_LeftStickDeadZone = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
	int32_t m_RightStickDeadZone = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
	int32_t m_TriggerDeadZone = XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
};

