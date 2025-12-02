#include "pch.h"
#include "ImGuiEx.h"
#include <imgui.h>

bool theatria::ImGuiEx::ColoredDragFloat3(const char* label, float* v, float v_speed, float v_min, float v_max, const char* format)
{
    ImGui::PushID(label); // 識別子をプッシュ（同じ名前のコントロールが競合しないようにする）

    float item_width = ImGui::CalcItemWidth(); // 現在の項目幅を取得
    float single_item_width = (item_width - ImGui::GetStyle().ItemSpacing.x * 2) / 3; // 3分割

    // X軸（赤背景）
    ImGui::PushItemWidth(single_item_width);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.5f, 0.2f, 0.2f, 1.0f)); // 赤背景
    bool x_changed = ImGui::DragFloat("##X", &v[0], v_speed, v_min, v_max, format);
    ImGui::PopStyleColor();
    ImGui::PopItemWidth();

    ImGui::SameLine(); // 横並び

    // Y軸（緑背景）
    ImGui::PushItemWidth(single_item_width);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.5f, 0.2f, 1.0f)); // 緑背景
    bool y_changed = ImGui::DragFloat("##Y", &v[1], v_speed, v_min, v_max, format);
    ImGui::PopStyleColor();
    ImGui::PopItemWidth();

    ImGui::SameLine(); // 横並び

    // Z軸（青背景）
    ImGui::PushItemWidth(single_item_width);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.5f, 1.0f)); // 青背景
    bool z_changed = ImGui::DragFloat("##Z", &v[2], v_speed, v_min, v_max, format);
    ImGui::PopStyleColor();
    ImGui::PopItemWidth();

    ImGui::SameLine();        // ラベルとスライダーを横並びにする
    ImGui::Text("%s", label); // ラベルを表示

    ImGui::PopID(); // 識別子をポップ

    return x_changed || y_changed || z_changed;
}
