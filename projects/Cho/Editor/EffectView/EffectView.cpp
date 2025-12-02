#include "pch.h"
#include "EffectView.h"
#include "Editor/EditorManager/EditorManager.h"

void EffectView::Initialize()
{
}

void EffectView::Update()
{
	Window();
}

void EffectView::Window()
{
    // ウィンドウのパディングをゼロに設定
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    // 移動を無効にするフラグ
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    // ウィンドウの開始
    ImGui::Begin("EffectView", nullptr, windowFlags);

    // ウィンドウ内で利用可能な領域のサイズを取得
    ImVec2 availableSize = ImGui::GetContentRegionAvail();
    // アスペクト比を計算してカメラに設定
    float newAspect = availableSize.x / availableSize.y;
    m_DebugCamera->SetAspect(newAspect);

    // テクスチャを描画
    //D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = m_EngineCommand->GetEffectEditTextureHandle();
    D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = m_EngineCommand->GetSceneTextureHandle();
    ImTextureID textureID = (ImTextureID)srvHandle.ptr;

    ImVec4 tintColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);  // アルファ値を無視
    ImVec4 borderColor = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    ImGui::Image(textureID, availableSize, ImVec2(0, 0), ImVec2(1, 1), tintColor, borderColor);

    // ウィンドウ上にカーソルがあるときにデバッグカメラを更新する
    if (ImGui::IsWindowHovered())
    {
        m_DebugCamera->Update();
    } else
    {
        m_DebugCamera->UpdateMatrix();
    }

    ImGui::End();

    // スタイルを元に戻す
    ImGui::PopStyleVar();
}
