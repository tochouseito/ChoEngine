#pragma once
#include <imgui.h>

namespace theatria
{
	class ImGuiUtility
	{
		public:
		// ImGuiのディスプレイサイズを更新
		static void DisplayResize(const float& width, const float& height)
		{
			ImGuiIO& io = ImGui::GetIO();
			io.DisplaySize = ImVec2(width, height);
		}

	};
}