#include "pch.h"
#include "MainMenu.h"
#include <imgui.h>
#include "OS/Windows/WinApp/WinApp.h"
#include "Editor/EditorManager/EditorManager.h"
#include "GameCore/GameCore.h"
#include "GameCore/Systems/SingleSystems.h"
#include "EngineCommand/EngineCommands.h"
#include "GameCore/GameObject/GameObject.h"
#include "Platform/FileSystem/FileSystem.h"
#include "Platform/InputManager/InputManager.h"
#include "Resources/ResourceManager/ResourceManager.h"

void MainMenu::Initialize()
{
}

void MainMenu::Update()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        Window();
        PopupScriptName();
        PopupNewSceneName();
        SettingWindow();
		BuildSettingWindow();
    }
}

void MainMenu::Window()
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
    MenuBar();
    // ツールバー
    m_Toolbar->Update();

    // ウィンドウ内にドックスペースを作成
    ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
    ImGui::End(); // ドックスペースウィンドウを終了
}

void MainMenu::MenuBar()
{
    // メニューバー
    if (ImGui::BeginMenuBar())
    {
        // ファイルメニュー
        FileMenu();

        // 編集メニュー
        EditMenu();

        // レイアウトメニューの追加(Imguiのレイアウト)
        LayoutMenu();

        // Engine情報を表示
        EngineInfoMenu();

		// Helpメニュー
		HelpMenu();

		// 設定メニュー
		SettingMenu();

		// ビルド設定メニュー
		BuildSettingMenu();

        // ワークスペース一覧
        //static const char* workspaces[] = { "Layout", "Scripting", "Modeling", "Animation", "Rendering" };
        static const char* workspaces[] = { "SceneEdit","EffectEdit"};
        static int currentWorkspace = 0;

        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x * 0.3f); // メニューの横に配置（位置調整）

        for (int i = 0; i < IM_ARRAYSIZE(workspaces); ++i)
        {
            ImGui::PushID(i);

            // タブ風ボタン
            ImGui::PushStyleColor(ImGuiCol_Button, i == currentWorkspace ? ImVec4(0.3f, 0.4f, 0.8f, 1.0f) : ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.5f, 0.9f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.3f, 0.7f, 1.0f));
            if (ImGui::Button(workspaces[i]))
            {
                currentWorkspace = i;
                m_EditorManager->SetWorkSpaceType(workspaces[i]);
            }
            ImGui::PopStyleColor(3);

            ImGui::PopID();
            ImGui::SameLine(); // 横並びにする
        }


        ImGui::EndMenuBar(); // メニューバーを終了
    }
}

void MainMenu::FileMenu()
{
    if (ImGui::BeginMenu("ファイル"))
    {
        // プロジェクトの保存
		if (ImGui::MenuItem("プロジェクトの保存"))
		{
			m_EngineCommand->SaveProjectFile();
		}
        if (m_EditorManager->GetWorkSpaceType() == WorkSpaceType::EffectEdit)
        {
            if (ImGui::MenuItem("エフェクトを保存"))
            {

            }
        }
        ImGui::EndMenu();
    }
}

void MainMenu::EditMenu()
{
    if (ImGui::BeginMenu("追加"))
    {
        switch (m_EditorManager->GetWorkSpaceType())
        {
        case WorkSpaceType::SceneEdit:
            if (ImGui::MenuItem("3Dオブジェクト"))
            {
                std::unique_ptr<Add3DObjectCommand> add3DObject = std::make_unique<Add3DObjectCommand>();
                m_EngineCommand->ExecuteCommand(std::move(add3DObject));
            }
            if (ImGui::MenuItem("カメラオブジェクト"))
            {
                std::unique_ptr<AddCameraObjectCommand> addCameraObject = std::make_unique<AddCameraObjectCommand>();
                m_EngineCommand->ExecuteCommand(std::move(addCameraObject));
            }
            if (ImGui::MenuItem("パーティクルシステムオブジェクト"))
            {
                std::unique_ptr<AddParticleSystemObjectCommand> addParticleSystemObject = std::make_unique<AddParticleSystemObjectCommand>();
                m_EngineCommand->ExecuteCommand(std::move(addParticleSystemObject));
            }
            if (ImGui::MenuItem("UI"))
            {
				std::unique_ptr<AddUIObjectCommand> addUIObject = std::make_unique<AddUIObjectCommand>();
				m_EngineCommand->ExecuteCommand(std::move(addUIObject));
            }
			if (ImGui::MenuItem("ライト"))
			{
				std::unique_ptr<AddLightObjectCommand> addLightObject = std::make_unique<AddLightObjectCommand>();
				m_EngineCommand->ExecuteCommand(std::move(addLightObject));
			}
            if (ImGui::MenuItem("スクリプト"))
            {
                m_OpenScriptPopup = true;
            }
            if (ImGui::MenuItem("シーン"))
            {
				// シーンを追加
				m_OpenScenePopup = true;
            }
            break;
		case WorkSpaceType::EffectEdit:
            if (ImGui::MenuItem("新規作成"))
            {
				// エフェクトを新規作成
				std::unique_ptr<CreateEffectCommand> addEffectObject = std::make_unique<CreateEffectCommand>();
				m_EngineCommand->ExecuteCommand(std::move(addEffectObject));
            }
            if (ImGui::MenuItem("ノードの追加"))
            {
				// 現在の編集中のRootにノードを追加
				std::unique_ptr<AddEffectNodeCommand> addEffectNode = std::make_unique<AddEffectNodeCommand>();
				m_EngineCommand->ExecuteCommand(std::move(addEffectNode));
            }
            break;
        default:
            break;
        }

    

        ImGui::EndMenu(); // 「Edit」メニューを終了
    }
}

void MainMenu::LayoutMenu()
{
    static bool showStyleEditor = false;
    static bool showMetricsWindow = false;
    static bool showDemoImGui = false;
    // ドッキング解除を無効にするフラグ
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking;
    if (ImGui::BeginMenu("レイアウト"))
    {
        if (ImGui::MenuItem("Show Style Editor"))
        {
            showStyleEditor = true;
        }
        if (ImGui::MenuItem("Show Metrics"))
        {
            showMetricsWindow = true;
        }
        if (ImGui::MenuItem("Show DemoWindow"))
        {
            showDemoImGui = true;
        }
        ImGui::EndMenu();
    }

    if (showStyleEditor)
    {
        ImGui::Begin("Style Editor", &showStyleEditor, windowFlags);
        ImGui::ShowStyleEditor();
        ImGui::End();
    }
    if (showMetricsWindow)
    {
        ImGui::ShowMetricsWindow(&showMetricsWindow);
    }
    if (showDemoImGui)
    {
        ImGui::ShowDemoWindow(&showDemoImGui);
    }
}

void MainMenu::EngineInfoMenu()
{
	if (ImGui::BeginMenu("Engine Info"))
	{
		//// TheatriaEngineのバージョンを表示
		//ImGui::Text("TheatriaEngine Version: %s", Cho::GetVersion().c_str());
		//ImGui::Separator();
		//// TheatriaEngineのビルド日を表示
		//ImGui::Text("Build Date: %s", __DATE__);
		//ImGui::EndMenu();
        // フレームレートを表示
        ImGuiIO& io = ImGui::GetIO();
        ImGui::Text("FPS: %.1f", io.Framerate);

		ECSManager* ecs = m_EngineCommand->GetGameCore()->GetECSManager();
		// double型msのECSのシステム計測時間を表示
		ImGui::Text("ECS Animation System Time: %.2f ms", ecs->GetLastSystemUpdateTimeMs<AnimationSystem>());

		ImGui::EndMenu();
	}
}

void MainMenu::HelpMenu()
{
    if (ImGui::BeginMenu("Help"))
    {
        if(ImGui::Button("https://tochouseito.github.io/TheatriaEngine_Docs/"))
        {
			WinApp::OpenWebURL(L"https://tochouseito.github.io/TheatriaEngine_Docs/");
        }
        
        ImGui::EndMenu();
    }
}

void MainMenu::SettingMenu()
{
	if (ImGui::BeginMenu("設定"))
	{
		if (ImGui::MenuItem("エディタ設定"))
		{
			// エディタ設定ウィンドウを開く
			m_OpenSettingWindow = true;
		}
		ImGui::EndMenu();
	}
}

void MainMenu::BuildSettingMenu()
{
	if (ImGui::BeginMenu("ビルド設定"))
	{
		if (ImGui::MenuItem("ビルド設定"))
		{
			// ビルド設定ウィンドウを開く
			m_OpenBuildSettingWindow = true;
		}
		ImGui::EndMenu();
	}
}

void MainMenu::PopupScriptName()
{
    // スクリプト名バッファ
    static char scriptNameBuffer[64] = "";
	static bool ScriptFocusInput = false; // フォーカス状態を保持
    if (m_OpenScriptPopup)
    {
        std::memset(scriptNameBuffer, 0, sizeof(scriptNameBuffer)); // 初期化
        ImGui::OpenPopup("ScriptNamePopup");
        m_OpenScriptPopup = false; // 一度だけ開くように
		ScriptFocusInput = true; // ポップアップが開かれたら入力フィールドにフォーカスを当てる
    }
    if (ImGui::BeginPopupModal("ScriptNamePopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("スクリプト名を入力してください（A-Z, a-z）:");
		// フォーカスを当てる
        if (ScriptFocusInput)
        {
            ImGui::SetKeyboardFocusHere(); // 次のInputTextにフォーカス
            ScriptFocusInput = false;
        }
        ImGui::InputText("##ScriptName", scriptNameBuffer, IM_ARRAYSIZE(scriptNameBuffer),
            ImGuiInputTextFlags_CallbackCharFilter,
            [](ImGuiInputTextCallbackData* data) -> int {
                if ((data->EventChar >= 'a' && data->EventChar <= 'z') ||
                    (data->EventChar >= 'A' && data->EventChar <= 'Z'))
                {
                    return 0;
                }
                return 1;
            });

        if (ImGui::Button("OK"))
        {
            std::string scriptName = scriptNameBuffer;
            m_EngineCommand->GenerateScript(scriptName);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("キャンセル"))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void MainMenu::PopupNewSceneName()
{
	// シーン名バッファ
	static char sceneNameBuffer[64] = "";
	static bool sceneFocusInput = false; // フォーカス状態を保持
	if (m_OpenScenePopup)
	{
		std::memset(sceneNameBuffer, 0, sizeof(sceneNameBuffer)); // 初期化
		ImGui::OpenPopup("SceneNamePopup");
		m_OpenScenePopup = false; // 一度だけ開くように
		sceneFocusInput = true; // ポップアップが開かれたら入力フィールドにフォーカスを当てる
	}
	if (ImGui::BeginPopupModal("SceneNamePopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("シーン名を入力してください（A-Z, a-z）:");
		// フォーカスを当てる
        if (sceneFocusInput)
        {
            ImGui::SetKeyboardFocusHere(); // 次のInputTextにフォーカス
            sceneFocusInput = false;
		}
		ImGui::InputText("##SceneName", sceneNameBuffer, IM_ARRAYSIZE(sceneNameBuffer),
			ImGuiInputTextFlags_CallbackCharFilter,
			[](ImGuiInputTextCallbackData* data) -> int {
				if ((data->EventChar >= 'a' && data->EventChar <= 'z') ||
					(data->EventChar >= 'A' && data->EventChar <= 'Z'))
				{
					return 0;
				}
				return 1;
			});
		if (ImGui::Button("OK"))
		{
			std::string sceneName = sceneNameBuffer;
            m_EngineCommand->GetGameCore()->GetSceneManager()->CreateDefaultScene(ConvertString(sceneName));
			m_EditorManager->ChangeEditingScene(ConvertString(sceneName));
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("キャンセル"))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void MainMenu::SettingWindow()
{
	// 設定ウィンドウ
    if (!m_OpenSettingWindow) { return; }
	ImGui::Begin("Setting##MainMenuSetting", &m_OpenSettingWindow);
	ImGui::Text("エディタ設定");

    ImGui::Text("重力");
    ImGui::SameLine();
    float3 gravity = m_EngineCommand->GetGameCore()->GetPhysicsWorld3D()->GetGravity();
    if (ImGui::InputFloat3("##Gravity", &gravity.x))
    {
		// 変更されたら重力を設定
		float3 gravityfloat3(gravity.x, gravity.y, gravity.z);
		std::unique_ptr<SetGravityCommand> setGravity = std::make_unique<SetGravityCommand>(gravityfloat3);
		m_EngineCommand->ExecuteCommand(std::move(setGravity));
    }

	ImGui::Text("コントローラー設定");
    int leftDeadZone = m_EngineCommand->GetInputManager()->GetLeftStickDeadZone();
    int rightDeadZone = m_EngineCommand->GetInputManager()->GetRightStickDeadZone();
	ImGui::DragInt("LeftStickDeadZone", &leftDeadZone, 1, 0, 10000);
	ImGui::DragInt("RightStickDeadZone", &rightDeadZone, 1, 0, 10000);
	m_EngineCommand->GetInputManager()->SetJoystickDeadZone(0,leftDeadZone, rightDeadZone);

	ImGui::Text("最初のシーン");
    static std::wstring selectFirstScene = theatria::FileSystem::g_GameSettings.startScene;
	std::wstring startScene = L"シーンがありません";
    if (!selectFirstScene.empty())
    {
		startScene = selectFirstScene;
    }
    if (ImGui::BeginCombo("##firstSceneSelector", ConvertString(startScene).c_str()))
    {
        for (const auto& scene : m_EngineCommand->GetGameCore()->GetSceneManager()->GetScenes())
        {
            if (ImGui::Selectable(ConvertString(scene.GetName()).c_str()))
            {
				selectFirstScene = scene.GetName(); // 選択されたシーン名を保存
                // ゲーム設定に保存
                theatria::FileSystem::g_GameSettings.startScene = selectFirstScene;
            }
        }
        ImGui::EndCombo();
    }
    ImGui::Text("シーンを切り替え");
    std::wstring sceneName = m_EditorManager->GetEditingSceneName();
	if (ImGui::BeginCombo("##SceneSelector", ConvertString(sceneName).c_str()))
	{
		for (const auto& scene : m_EngineCommand->GetGameCore()->GetSceneManager()->GetScenes())
		{
            if (ImGui::Selectable(ConvertString(scene.GetName()).c_str()))
            {
                if (sceneName != scene.GetName())
                {
					m_EditorManager->ChangeEditingScene(scene.GetName());
                    m_EditorManager->SetSelectedGameObject(nullptr);
                }
            }
		}
		ImGui::EndCombo();
	}

	// SkyboxTextureの選択
	ImGui::Text("Skybox Texture");
	std::wstring skyboxTextureName = L"テクスチャがありません";
    if (!m_EngineCommand->GetResourceManager()->GetSkyboxTextureName().empty())
    {
		skyboxTextureName = m_EngineCommand->GetResourceManager()->GetSkyboxTextureName();
    }
    std::unordered_map<std::wstring, uint32_t>& textureMap = m_EngineCommand->GetResourceManager()->GetTextureManager()->GetTextureNameContainer();

    if(ImGui::BeginCombo("##SkyboxTextureSelector", ConvertString(skyboxTextureName).c_str()))
    {
        for (const auto& texture : textureMap)
        {
            if (!m_EngineCommand->GetResourceManager()->GetTextureManager()->GetTextureData(texture.first)->metadata.IsCubemap())
            {
				continue; // キューブマップでないテクスチャはスキップ
            }
            if (ImGui::Selectable(ConvertString(texture.first).c_str()))
            {
                m_EngineCommand->GetResourceManager()->SetSkyboxTextureName(texture.first);
            }
        }
        ImGui::EndCombo();
	}

    ImGui::End();
}

void MainMenu::BuildSettingWindow()
{
	// ビルド設定ウィンドウ
	if (!m_OpenBuildSettingWindow) { return; }
	ImGui::Begin("BuildSetting##MainMenuBuildSetting", &m_OpenBuildSettingWindow);
	ImGui::Text("ビルド設定");
    static char titleBarBuffer[256];
    if (ImGui::Button("タイトルバーを入力"))
    {
        ImGui::OpenPopup("TitleBarInputPopup");
        // 初期値を UTF-8 にして C バッファへ
        std::memset(titleBarBuffer, 0, sizeof(titleBarBuffer));
        const std::string u8 = ConvertString(theatria::FileSystem::g_GameSettings.titleBar);
        std::memcpy(titleBarBuffer, u8.c_str(), std::min(u8.size(), sizeof(titleBarBuffer) - 1));
    }
    if (ImGui::BeginPopupModal("TitleBarInputPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("タイトルバーを入力してください:");
        ImGui::InputText("##TitleBar", titleBarBuffer, IM_ARRAYSIZE(titleBarBuffer));
        if (ImGui::Button("OK"))
        {
            // UTF-8 → UTF-16 に戻して保存
            std::wstring newTitleBar = ConvertString(std::string(titleBarBuffer));
            theatria::FileSystem::g_GameSettings.titleBar = newTitleBar;

            // もし Win32 ウィンドウタイトルも即時反映したいなら:
            // ::SetWindowTextW(hwnd, newTitleBar.c_str());

            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("キャンセル"))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    static char exeNameBuffer[256];
    if (ImGui::Button("実行ファイル名を入力"))
    {
        ImGui::OpenPopup("ExeNameInputPopup");
        // 初期値を UTF-8 にして C バッファへ
        std::memset(exeNameBuffer, 0, sizeof(exeNameBuffer));
        const std::string u8 = ConvertString(theatria::FileSystem::g_GameSettings.exeName);
        std::memcpy(exeNameBuffer, u8.c_str(), std::min(u8.size(), sizeof(exeNameBuffer) - 1));
    }
    if (ImGui::BeginPopupModal("ExeNameInputPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("実行ファイル名を入力してください:");
        ImGui::InputText("##ExeName", exeNameBuffer, IM_ARRAYSIZE(exeNameBuffer));
        if (ImGui::Button("OK"))
        {
            // UTF-8 → UTF-16 に戻して保存
            std::wstring newExeName = ConvertString(std::string(exeNameBuffer));
            theatria::FileSystem::g_GameSettings.exeName = newExeName;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("キャンセル"))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (ImGui::Button("フォルダを選択してビルド"))
    {
        std::wstring folderPath;
        folderPath = theatria::FileSystem::GameBuilder::SelectFolderDialog();
		if (!folderPath.empty())
		{
			// フォルダが選択された場合、ビルドを実行
            FileSystem::GameBuilder::CopyFilesToBuildFolder(m_EngineCommand,folderPath);
        }
    }
	
	ImGui::End();
}
