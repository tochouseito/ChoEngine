#include "pch.h"
#include "GameView.h"
#include "OS/Windows/WinApp/WinApp.h"
#include "Editor/EditorManager/EditorManager.h"
#include "EngineCommand/EngineCommand.h"

void GameView::Initialize()
{
}

void GameView::Update()
{
	Window();
}

void GameView::Window()
{
    // ウィンドウのパディングをゼロに設定
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    // 移動を無効にするフラグ
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    // ウィンドウの開始
    ImGui::Begin("GameView", nullptr, windowFlags);

    // ウィンドウ内の描画可能領域を取得
    ImVec2 contentRegion = ImGui::GetContentRegionAvail();

    // 固定解像度のテクスチャサイズ
    ImVec2 textureResolution = {
        static_cast<float>(m_EngineCommand->GetGameResolutionX()),
        static_cast<float>(m_EngineCommand->GetGameResolutionY())
    };
    // アスペクト比を維持しつつ、ウィンドウ内の描画可能領域に収まるようにスケーリング係数を計算
    float scaleX = contentRegion.x / textureResolution.x;
    float scaleY = contentRegion.y / textureResolution.y;
    float scale = std::min(scaleX, scaleY);

    // スケーリング後のテクスチャサイズを計算
    ImVec2 scaledTextureSize = ImVec2(textureResolution.x * scale, textureResolution.y * scale);

    // 描画位置のオフセットを計算（ウィンドウの中央に配置するため）
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    float offsetX = cursorPos.x + (contentRegion.x - scaledTextureSize.x) * 0.5f;
    float offsetY = cursorPos.y + (contentRegion.y - scaledTextureSize.y) * 0.5f;

    // カーソル位置を設定し、スクリーン上にテクスチャを中央に描画
    ImGui::SetCursorScreenPos(ImVec2(offsetX, offsetY));

    // テクスチャの描画
	D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = m_EngineCommand->GetGameTextureHandle();
    ImTextureID textureID = (ImTextureID)srvHandle.ptr;

    ImVec4 tintColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);  // アルファ値を無視
    ImVec4 borderColor = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    ImGui::Image(textureID, scaledTextureSize, ImVec2(0, 0), ImVec2(1, 1), tintColor, borderColor);

    //// "Scene View" 上にカーソルがあるとき、右クリックメニューを開かないようにする
    //if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
    //{
    //    excludeRightClickMenu = true;
    //} else
    //{
    //    excludeRightClickMenu = false;
    //}

    // ウィンドウの終了
    ImGui::End();

    // スタイルを元に戻す
    ImGui::PopStyleVar();
}
