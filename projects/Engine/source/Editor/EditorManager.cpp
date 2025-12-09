#include "pch.h"
#ifndef NDEBUG
#include "include/Editor/EditorManager.h"
#include "include/Core/FrameCounter.h"

#include <imgui.h>

using namespace Theatria::Editor;

bool EditorManager::Initialize(Core::FrameCounter* fc, Graphics::DescriptorAllocator* da, Graphics::FrameGraph* fg)
{
    m_pFrameCounter = fc;

    m_AssetBrowser = std::make_unique<AssetBrowser>();
    m_AssetBrowser->Initialize();
    m_GameView = std::make_unique<GameView>();
    m_GameView->Initialize();
    m_SceneView = std::make_unique<SceneView>(da, fg);
    m_SceneView->Initialize();
    m_Hierarchy = std::make_unique<Hierarchy>();
    m_Hierarchy->Initialize();
    m_Inspector = std::make_unique<Inspector>();
    m_Inspector->Initialize();

    return true;
}

void EditorManager::Shutdown()
{
    // シャットダウン処理
}

void EditorManager::Update()
{
    BackDockingWindows();

    m_AssetBrowser->Update();
    m_GameView->Update();
    m_SceneView->Update();
    m_Hierarchy->Update();
    m_Inspector->Update();
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
    // フレームレートを表示
    ImGuiIO& io = ImGui::GetIO();
    ImGui::Text("IMGUI FPS: %.1f", io.Framerate);
    ImGui::Text("ENGINE FPS: %.1f", static_cast<float>(m_pFrameCounter->FPS()));
    ImGui::End();
}

#endif // !NDEBUG
