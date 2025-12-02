#include "pch.h"
#ifndef NDEBUG
#include "include/Editor/EditorManager.h"
#include <imgui.h>

using namespace Theatria::Editor;

bool EditorManager::Initialize()
{
    // 初期化処理
    return true;
}

void EditorManager::Shutdown()
{
    // シャットダウン処理
}

void EditorManager::Update()
{
    BackDockingWindows();
}

void EditorManager::BackDockingWindows()
{
    // ビューポート全体をカバーするドックスペースを作成
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos); // 次のウィンドウの位置をメインビューポートの位置に設定
    ImGui::SetNextWindowSize(viewport->Size); // 次のウィンドウのサイズをメインビューポートのサイズに設定
    ImGui::SetNextWindowViewport(viewport->ID); // ビューポートIDをメインビューポートに設定

    // タイトルバーを削除し、リサイズや移動を防止し、背景のみとするウィンドウフラグを設定
    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_MenuBar;

    // ウィンドウの丸みとボーダーをなくして、シームレスなドッキング外観にする
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImGui::Begin("DockSpace Window", nullptr, window_flags); // ドックスペースとして機能する新しいウィンドウを開始
    ImGui::PopStyleVar(2); // 先ほどプッシュしたスタイル変数を2つポップする

    // メニューバー
    //MenuBar();
    //// ツールバー
    //m_Toolbar->Update();

    // ウィンドウ内にドックスペースを作成
    ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
    ImGui::End(); // ドックスペースウィンドウを終了

    ImGui::Begin("EditorManager Debug Info");
    ImGui::Text("EditorManager is running.");
    ImGui::End();
}

#endif // !NDEBUG
