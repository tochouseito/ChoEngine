#include "pch.h"
#include "InputManager.h"
#include "OS/Windows/WinApp/WinApp.h"
#include "TheatriaEngineAPI.h"

void InputManager::Initialize()
{
	HRESULT hr;

	// DirectInoutの初期化
	hr = DirectInput8Create(
		WinApp::GetHInstance(), DIRECTINPUT_VERSION, IID_IDirectInput8,
		(void**)&m_DirectInput, nullptr);
	Log::Write(LogLevel::Assert, "DirectInput8 created.", hr);

	// キーボードデバイスの生成
	hr = m_DirectInput->CreateDevice(GUID_SysKeyboard, &m_Keyboard, NULL);
	Log::Write(LogLevel::Assert, "Keyboard created.", hr);

	// 入力データの形式のセット
	hr = m_Keyboard->SetDataFormat(&c_dfDIKeyboard);// 標準形式
	Log::Write(LogLevel::Assert, "Set data format.", hr);

	// 排他制御レベルのセット
	hr = m_Keyboard->SetCooperativeLevel(
		WinApp::GetHWND(), DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
	Log::Write(LogLevel::Assert, "Set cooperative level.", hr);

	/*マウスデバイスの生成*/
	hr = m_DirectInput->CreateDevice(GUID_SysMouse, &m_DIMouse, NULL);
	Log::Write(LogLevel::Assert, "Mouse created.", hr);

	/*入力データの形式のセット*/
	hr = m_DIMouse->SetDataFormat(&c_dfDIMouse2);/*標準形式*/
	Log::Write(LogLevel::Assert, "Set data format.", hr);

	/*排他制御レベルのセット*/
	hr = m_DIMouse->SetCooperativeLevel(
		WinApp::GetHWND(), DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
	Log::Write(LogLevel::Assert, "Set cooperative level.", hr);

	// XInputパッドの準備（4つのパッドに対応）
	for (int i = 0; i < 4; ++i)
	{
		Joystick joystick = {};
		joystick.type = PadType::XInput;
		m_Joysticks.push_back(joystick);
	}
}

void InputManager::Update()
{
	// 前回のキー入力を保存
	memcpy(m_PreKey, m_Key, sizeof(m_Key));
	/*マウス*/
	m_MousePre = m_Mouse;

	// 現在のウィンドウがフォーカスされているかを確認
	if (GetForegroundWindow() == WinApp::GetHWND())
	{

		// キーボード情報の取得開始
		m_Keyboard->Acquire();
		/*マウス*/
		m_DIMouse->Acquire();

		// 全キーの入力状態を取得する
		m_Keyboard->GetDeviceState(sizeof(m_Key), m_Key);
		m_DIMouse->GetDeviceState(sizeof(m_Mouse), &m_Mouse);

		// マウス位置の取得
		POINT point;
		GetCursorPos(&point);
		ScreenToClient(WinApp::GetHWND(), &point);
		m_MousePosition = float2(static_cast<float>(point.x), static_cast<float>(point.y));

		// XInputコントローラーの入力取得
		for (int i = 0; i < 4; ++i)
		{
			Joystick& joystick = m_Joysticks[i];

			if (joystick.type == PadType::XInput)
			{
				joystick.statePre = joystick.state; // 前回の状態を保存

				// XInputのステートを取得
				if (XInputGetState(i, &joystick.state.xInput) == ERROR_SUCCESS)
				{
					joystick.statePre = joystick.state;

					// 左スティック（補正値で上書き）
					float2 leftStick = ApplyRadialDeadZone(
						joystick.state.xInput.Gamepad.sThumbLX,
						joystick.state.xInput.Gamepad.sThumbLY,
						m_LeftStickDeadZone);

					joystick.state.xInput.Gamepad.sThumbLX = static_cast<SHORT>(leftStick.x * 32767.0f);
					joystick.state.xInput.Gamepad.sThumbLY = static_cast<SHORT>(leftStick.y * 32767.0f);

					// 右スティック
					float2 rightStick = ApplyRadialDeadZone(
						joystick.state.xInput.Gamepad.sThumbRX,
						joystick.state.xInput.Gamepad.sThumbRY,
						m_RightStickDeadZone);

					joystick.state.xInput.Gamepad.sThumbRX = static_cast<SHORT>(rightStick.x * 32767.0f);
					joystick.state.xInput.Gamepad.sThumbRY = static_cast<SHORT>(rightStick.y * 32767.0f);

					// トリガー（通常処理）
					BYTE& leftTrigger = joystick.state.xInput.Gamepad.bLeftTrigger;
					BYTE& rightTrigger = joystick.state.xInput.Gamepad.bRightTrigger;

					if (leftTrigger < m_TriggerDeadZone)
					{
						leftTrigger = 0;
					}
					if (rightTrigger < m_TriggerDeadZone)
					{
						rightTrigger = 0;
					}
				}
			}
		}
	} else
	{
		// フォーカスが外れている場合はキーとマウスの状態をリセット
		memset(m_Key, 0, sizeof(m_Key));
		memset(&m_Mouse, 0, sizeof(m_Mouse));
	}
}

bool InputManager::PushKey(const uint8_t& keyNumber) const
{
	if (m_Key[keyNumber])
	{
		return true;
	}
	return false;
}

bool InputManager::TriggerKey(const uint8_t& keyNumber) const
{
	if (m_Key[keyNumber] && !m_PreKey[keyNumber])
	{
		return true;
	}
	return false;
}

const DIMOUSESTATE2& InputManager::GetAllMouse() const
{
	return m_Mouse;
}

bool InputManager::IsPressMouse(const int32_t& mouseNumber) const
{
	return m_Mouse.rgbButtons[mouseNumber] & 0x80;
}

bool InputManager::IsTriggerMouse(const int32_t& buttonNumber) const
{
	return (m_Mouse.rgbButtons[buttonNumber] & 0x80) && !(m_MousePre.rgbButtons[buttonNumber] & 0x80);
}

MouseMove InputManager::GetMouseMove()
{
	MouseMove move = { m_Mouse.lX, m_Mouse.lY, m_Mouse.lZ };
	return move;
}

int32_t InputManager::GetWheel() const
{
	return m_Mouse.lZ;
}

const float2& InputManager::GetMouseWindowPosition() const
{
	return m_MousePosition;
}

float2 InputManager::GetMouseScreenPosition() const
{
	// マウス位置の取得
	POINT point;
	GetCursorPos(&point);
	float2 result = float2(static_cast<float>(point.x), static_cast<float>(point.y));
	return result;
}

bool InputManager::GetJoystickState(const int32_t& stickNo, XINPUT_STATE& out) const
{
	if (stickNo < 0 || stickNo >= m_Joysticks.size()) return false;
	if (m_Joysticks[stickNo].type != PadType::XInput) return false;

	out = m_Joysticks[stickNo].state.xInput;
	return true;
}

bool InputManager::GetJoystickStatePrevious(const int32_t& stickNo, XINPUT_STATE& out) const
{
	if (stickNo < 0 || stickNo >= m_Joysticks.size()) return false;
	if (m_Joysticks[stickNo].type != PadType::XInput) return false;

	out = m_Joysticks[stickNo].statePre.xInput;
	return true;
}

void InputManager::SetJoystickDeadZone(const int32_t& stickNo, const int32_t& deadZoneL, const int32_t& deadZoneR)
{
	stickNo;
	m_LeftStickDeadZone = deadZoneL;
	m_RightStickDeadZone = deadZoneR;
}

size_t InputManager::GetNumberOfJoysticks()
{
	return size_t();
}

bool InputManager::IsTriggerPadButton(const PadButton& button, int32_t stickNo)
{
	XINPUT_STATE currentState;
	XINPUT_STATE previousState;

	if (button == PadButton::LTrigger or button == PadButton::RTrigger)
	{
		return IsTriggerTrigger(button == PadButton::LTrigger ? LR::LEFT : LR::RIGHT, stickNo);

	} else
	{
		if (GetJoystickState(stickNo, currentState) && GetJoystickStatePrevious(stickNo, previousState))
		{
			return (
				(currentState.Gamepad.wButtons & (WORD)button) != 0) &&
				((previousState.Gamepad.wButtons & (WORD)button) == 0
					);
		}
	}

	return false;
}

bool InputManager::IsPressPadButton(const PadButton& button, int32_t stickNo)
{
	XINPUT_STATE currentState;

	if (button == PadButton::LTrigger or button == PadButton::RTrigger)
	{
		return GetLRTrigger(button == PadButton::LTrigger ? LR::LEFT : LR::RIGHT, stickNo) > 0.8f;

	} else
	{
		if (GetJoystickState(stickNo, currentState))
		{
			// 押されている場合は true を返す
			return (currentState.Gamepad.wButtons & (WORD)button) != 0;
		}
	}
	return false;
}

float2 InputManager::GetStickValue(const LR& padStick, int32_t stickNo)
{
	float2 result{ 0.0f,0.0f };

	XINPUT_STATE currentState;
	if (!GetJoystickState(stickNo, currentState))
	{
		return { 0.0f, 0.0f };
	}
	if (GetJoystickState(stickNo, currentState))
	{
		switch (padStick)
		{
		case LR::LEFT:
			return ApplyRadialDeadZone(
				currentState.Gamepad.sThumbLX,
				currentState.Gamepad.sThumbLY,
				m_LeftStickDeadZone);
		case LR::RIGHT:
			return ApplyRadialDeadZone(
				currentState.Gamepad.sThumbRX,
				currentState.Gamepad.sThumbRY,
				m_RightStickDeadZone);
			break;
		}
	}

	return result;
}

float InputManager::GetLRTrigger(const LR& LorR, int32_t stickNo)
{
	XINPUT_STATE currentState;
	float value = 0;

	if (GetJoystickState(stickNo, currentState))
	{
		if (LorR == LR::LEFT)
		{
			value = static_cast<float>(currentState.Gamepad.bLeftTrigger) / 255.0f;
		} else
		{
			value = static_cast<float>(currentState.Gamepad.bRightTrigger) / 255.0f;
		}
	}

	return value;
}

float2 InputManager::CheckAndWarpMouse()
{
	if (IsPressMouse(Left) ||
		IsPressMouse(Right) ||
		IsPressMouse(Center))
	{

		// 画面の解像度を取得
		RECT screenRect;
		GetClientRect(GetDesktopWindow(), &screenRect);
		int screenWidth = screenRect.right;
		int screenHeight = screenRect.bottom;

		// マウスカーソルの現在位置を取得
		POINT cursorPos;
		GetCursorPos(&cursorPos);

		// マウスの相対移動量（補正用）
		float deltaX = 0.0f;
		float deltaY = 0.0f;

		// 画面端でワープ処理
		if (cursorPos.x <= 0)
		{
			deltaX = static_cast<float>(screenWidth - 2); // ワープによる移動量
			SetCursorPos(screenWidth - 2, cursorPos.y);
		} else if (cursorPos.x >= screenWidth - 1)
		{
			deltaX = static_cast<float>(-screenWidth + 2);
			SetCursorPos(1, cursorPos.y);
		}

		if (cursorPos.y <= 0)
		{
			deltaY = static_cast<float>(screenHeight - 2);
			SetCursorPos(cursorPos.x, screenHeight - 2);
		} else if (cursorPos.y >= screenHeight - 1)
		{
			deltaY = static_cast<float>(-screenHeight + 2);
			SetCursorPos(cursorPos.x, 1);
		}

		// マウスの相対移動量を補正
		float2 result = { deltaX,deltaY };
		return result;
	} else
	{
		return { 0.0f,0.0f };
	}

}

bool InputManager::IsTriggerTrigger(const LR& LorR, int32_t stickNo)
{
	XINPUT_STATE currentState;
	XINPUT_STATE previousState;
	float value[2] = { 0.0f };

	if (GetJoystickState(stickNo, currentState))
	{
		if (LorR == LR::LEFT)
		{
			value[0] = static_cast<float>(currentState.Gamepad.bLeftTrigger) / 255.0f;
		} else
		{
			value[0] = static_cast<float>(currentState.Gamepad.bRightTrigger) / 255.0f;
		}
	}

	if (GetJoystickStatePrevious(stickNo, previousState))
	{
		if (LorR == LR::LEFT)
		{
			value[1] = static_cast<float>(previousState.Gamepad.bLeftTrigger) / 255.0f;
		} else
		{
			value[1] = static_cast<float>(previousState.Gamepad.bRightTrigger) / 255.0f;
		}
	}

	return value[0] > 0.8f && value[1] <= 0.8f;
}

float2 InputManager::ApplyRadialDeadZone(int16_t rawX, int16_t rawY, int deadZone)
{
	float x = static_cast<float>(rawX);
	float y = static_cast<float>(rawY);
	float magnitude = std::sqrt(x * x + y * y);

	if (magnitude < static_cast<float>(deadZone))
	{
		return float2(0.0f, 0.0f);
	}

	float norm = (magnitude - deadZone) / (32767.0f - deadZone);
	norm = std::clamp(norm, 0.0f, 1.0f);

	float scale = norm / magnitude;

	return float2(x * scale, y * scale);
}
