#include "pch.h"
#include "Toolbar.h"
#include "Editor/EditorManager/EditorManager.h"
#include "GameCore/GameCore.h"
#include "Core/ChoLog/ChoLog.h"
#include "Core/Utility/FontCode.h"

void Toolbar::Initialize()
{
}

void Toolbar::Update()
{
	Window();
}

void Toolbar::Window()
{
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 2));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(40, 40, 40, 255)); // 背景色

    // ビューポートの横幅を取得して横いっぱいにツールバーを描画
    ImGui::BeginChild("Toolbar", ImVec2(0, 32), false, ImGuiWindowFlags_NoScrollbar);

    // ツールボタン（アイコン風）
    //if (ImGui::Button("☀")) { /* Scene Tool */ }
    //ImGui::SameLine();
    //if (ImGui::Button("🡆")) { /* Move Tool */ }
    //ImGui::SameLine();
    //if (ImGui::Button("⤾")) { /* Rotate Tool */ }
    //ImGui::SameLine();
    //if (ImGui::Button("⇲")) { /* Scale Tool */ }
    ImGui::SameLine(0, 40); // 大きめにスペース
    // 上部に余白（例：4px）
    ImGui::Dummy(ImVec2(1, 4));

    // 左側にプルダウン（Combo）
    ImGui::SetCursorPosX(8); // 左端に寄せる
    std::wstring name = L"カメラがありません！";
	GameObject* mainCamera = m_EngineCommand->GetGameCore()->GetGameWorld()->GetMainCamera();
    if (mainCamera)
    {
        // オブジェクトの名前を取得
        name = mainCamera->GetName();
    }
    ImGui::SetNextItemWidth(100); // プルダウンの横幅指定
    if (ImGui::BeginCombo("##ToolSelector", ConvertString(name).c_str()))
    {
        for (const auto& scene : m_EngineCommand->GetGameCore()->GetGameWorld()->GetWorldContainer())
        {
            for (const auto& objects : scene)
            {
                for (const auto& object : objects)
                {
                    if(object->GetType() != ObjectType::Camera) {
                        continue; // カメラ以外はスキップ
					}
					bool is_selected = (object->GetName() == name);
                    if (ImGui::Selectable(ConvertString(object->GetName()).c_str(), is_selected))
                    {
                        m_EngineCommand->GetGameCore()->GetGameWorld()->SetMainCamera(object.get());
                    }
                    if (is_selected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
            }
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    // 中央に配置するための計算
    float toolbarWidth = 0.0f;
    float buttonSize = 28.0f; // ボタンサイズ
    float buttonSpacing = 8.0f; // ボタン間のスペース
    int buttonCount = 3;
    toolbarWidth = buttonCount * buttonSize + (buttonCount - 1) * buttonSpacing;

    // ウィンドウ中央位置に移動
    float availableWidth = ImGui::GetContentRegionAvail().x;
    float startX = (availableWidth - toolbarWidth) * 0.5f;
    ImGui::SetCursorPosX(startX);

    // ゲーム実行中なら
    if (m_EngineCommand->IsGameRunning())
    {
        // ボタンを描画（中央に並べる）
        if (ImGui::Button(ICON_FA_PAUSE, ImVec2(buttonSize, buttonSize)))
        {
            /* Pause */
            m_EngineCommand->GameStop();
            // 編集中のシーンをロード
            m_EditorManager->ReloadEditingScene();
        }
    } else // 実行中じゃないなら
    {
        // ボタンを描画（中央に並べる）
        if (ImGui::Button(ICON_FA_PLAY, ImVec2(buttonSize, buttonSize)))
        {
            // Play
			// 現在のSceneを保存
            m_EditorManager->SaveEditingScene();
            m_EngineCommand->GameRun(false);
        }
        // 横並び
		ImGui::SameLine();
        if (ImGui::Button("デバッガー実行"))
        {
            // Play
            // 現在のSceneを保存
            m_EditorManager->SaveEditingScene();
            m_EngineCommand->GameRun(true);
        }
    }
    ImGui::SameLine(0, buttonSpacing);
    if (ImGui::Button(ICON_FA_STEP_FORWARD, ImVec2(buttonSize, buttonSize))) { /* Step */ }

    ImGui::SameLine(0, 40);

    /*ImGui::Text("Layout:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    static int currentLayout = 0;
    const char* layouts[] = { "Default", "2D", "Debug" };
    ImGui::Combo("##Layout", &currentLayout, layouts, IM_ARRAYSIZE(layouts));*/

    ImGui::EndChild();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}
