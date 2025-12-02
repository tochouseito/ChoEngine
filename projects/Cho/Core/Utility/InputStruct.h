#pragma once
#include <cstdint>
#include <memory>
#include <wrl.h>
#include <Xinput.h>
#include <dinput.h>
enum class PadButton
{
	A = XINPUT_GAMEPAD_A,
	B = XINPUT_GAMEPAD_B,
	X = XINPUT_GAMEPAD_X,
	Y = XINPUT_GAMEPAD_Y,
	Up = XINPUT_GAMEPAD_DPAD_UP,
	Down = XINPUT_GAMEPAD_DPAD_DOWN,
	Left = XINPUT_GAMEPAD_DPAD_LEFT,
	Right = XINPUT_GAMEPAD_DPAD_RIGHT,
	Start = XINPUT_GAMEPAD_START,
	Back = XINPUT_GAMEPAD_BACK,
	LThumb = XINPUT_GAMEPAD_LEFT_THUMB,
	RThumb = XINPUT_GAMEPAD_RIGHT_THUMB,
	LShoulder = XINPUT_GAMEPAD_LEFT_SHOULDER,
	RShoulder = XINPUT_GAMEPAD_RIGHT_SHOULDER,
	LTrigger = VK_PAD_LTRIGGER,
	RTrigger = VK_PAD_RTRIGGER,
};
enum MouseButton
{
	Left = 0,
	Right,
	Center,
	Extra1,
	Extra2,
};

enum class LR
{
	LEFT = 0,
	RIGHT
};
struct MouseMove
{
	LONG lX;
	LONG lY;
	LONG lZ;
};
enum class PadType
{
	DirectInput,
	XInput,
};
// variantがC++17から
union State
{
	XINPUT_STATE xInput;
	DIJOYSTATE2 directInput;
};
struct Joystick
{
	Microsoft::WRL::ComPtr<IDirectInputDevice8> device;
	int32_t deadZoneL;
	int32_t deadZoneR;
	PadType type;
	State state;
	State statePre;
};