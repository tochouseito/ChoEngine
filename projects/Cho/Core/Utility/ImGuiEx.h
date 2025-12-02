#pragma once
namespace theatria
{
	// カスタムImGui
	namespace ImGuiEx
	{
		bool ColoredDragFloat3(const char* label, float* v, float v_speed, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f");

		// 
	};
}