#include "pch.h"
#ifndef NDEBUG
#include "include/Editor/SceneView.h"
#include <imgui.h>
#include "include/Graphics/FrameGraph.h"
#include "include/Graphics/DescriptorAllocator.h"
#include "include/Graphics/ResourceManager.h"

void Theatria::Editor::SceneView::Initialize()
{
}

void Theatria::Editor::SceneView::Update()
{
    // ウィンドウのパディングをゼロに設定
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    // 移動を無効にするフラグ
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    // ウィンドウの開始
    ImGui::Begin("Scene View", nullptr, windowFlags);

    // ウィンドウ内で利用可能な領域のサイズを取得
    ImVec2 availableSize = ImGui::GetContentRegionAvail();
    ImVec2 contentPos = ImGui::GetCursorScreenPos(); // コンテンツの位置を取得
    // m_Manipulate->SetContent(float2(contentPos.x, contentPos.y), float2(availableSize.x, availableSize.y));

    //ImVec2 windowPos = ImGui::GetWindowPos(); // 現在のImGuiウィンドウの位置を取得
    //float diffY = ImGui::GetWindowSize().y- ImGui::GetContentRegionAvail().y;
    //windowPos.y += diffY;
    //ImVec2 windowSize = ImGui::GetContentRegionAvail(); // 現在のImGuiウィンドウのサイズを取得

    //float2 vec = CheckAndWarpMouseInImGuiWindow();
    /*if (vec.x != 0.0f || vec.y != 0.0f) {

    }*/
    //debugCamera_->SetCurrentMousePos(vec);
    // アスペクト比を計算してカメラに設定
    // float newAspect = availableSize.x / availableSize.y;
    // m_DebugCamera->SetAspect(newAspect);

    // テクスチャを描画
    Graphics::ResourceHandle h = m_FrameGraph->FindResourceHandle("finalColor");
    Graphics::VirtualResource vr = m_FrameGraph->GetVirtualResource(h);
    D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = m_DescriptorAllocator->GetGPUHandle(vr.srvTableId);
    ImTextureID textureID = (ImTextureID)srvHandle.ptr;

    ImVec4 tintColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);  // アルファ値を無視
    ImVec4 borderColor = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    ImGui::Image(textureID, availableSize, ImVec2(0, 0), ImVec2(1, 1), tintColor, borderColor);

    //// "Debug View" 上にカーソルがあるとき、右クリックメニューを開かないようにする
    //if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup) && !excludeRightClickMenu)
    //{
    //    excludeRightClickMenu = true;
    //    if (inputManager_->IsPressMouse(Left) ||
    //        inputManager_->IsPressMouse(Right) ||
    //        inputManager_->IsPressMouse(Center))
    //    {
    //        if (inputManager_->IsTriggerMouse(Left) ||
    //            inputManager_->IsTriggerMouse(Right) ||
    //            inputManager_->IsTriggerMouse(Center))
    //        {
    //            // 初回クリック時にロック位置を決定
    //            GetCursorPos(&lockPosition);
    //            prevCursorPos = lockPosition;
    //            ShowCursor(false);
    //        }
    //        windowClick = true;
    //        setMousePos = true;
    //    } else
    //    {
    //        windowClick = false;
    //    }
    //} else
    //{
    //    excludeRightClickMenu = false;
    //}

    // ウィンドウ上にカーソルがあるときにデバッグカメラを更新する
    /*if (ImGui::IsWindowHovered())
    {
        m_DebugCamera->Update();
    }
    else
    {
        m_DebugCamera->UpdateMatrix();
    }

    m_Manipulate->Update();*/

    ImGui::End();

    // スタイルを元に戻す
    ImGui::PopStyleVar();
}

#endif // !NDEBUG
