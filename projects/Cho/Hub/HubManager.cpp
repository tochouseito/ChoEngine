#include "pch.h"
#include "HubManager.h"
#include <imgui.h>
#include "Platform/FileSystem/FileSystem.h"
#include "Core/ChoLog/ChoLog.h"
#include "Resources/ResourceManager/ResourceManager.h"
#include "Graphics/GraphicsEngine/GraphicsEngine.h"
#include "GameCore/GameCore.h"
#include "Editor/EditorManager/EditorManager.h"
#include "EngineCommand/EngineCommand.h"
#include "OS/Windows/WinApp/WinApp.h"
using namespace theatria;

void HubManager::Initialize()
{
    // キャッシュの読み込み
    FileSystem::LoadCacheFile(std::filesystem::current_path().wstring());
    if (m_IsGameRuntime)
    {
        // 起動Configの読み込み
        //std::wstring path = L"C:\\TheatriaGames\\PortfolioPiece";
        //LaunchConfig config = FileSystem::LoadLaunchConfig(path);
        LaunchConfig config = FileSystem::LoadLaunchConfig(std::filesystem::current_path().wstring());
        FileSystem::m_sProjectName = config.projectName;
    }
}

void HubManager::Update()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        Window();
    }
    if (m_IsGameRuntime)
    {
        //std::wstring selectedProjectName = FileSystem::m_sProjectName;
        // プロジェクトの読み込み
        FileSystem::LoadProjectFolder(std::filesystem::current_path().wstring(), m_pEngineCommand);
        //std::wstring path = L"C:\\TheatriaGames\\PortfolioPiece";
        //FileSystem::LoadProjectFolder(path, m_pEngineCommand);
        // プロジェクトのパスを保存
        //FileSystem::ScriptProject::LoadProjectPath(selectedProjectName);
        // ブランチを取得
        //GetCurrentBranch();
        // ウィンドウのタイトルバーを変更
        SetWindowTextW(WinApp::GetHWND(), FileSystem::g_GameSettings.titleBar.c_str());
        // プロジェクト選択後、Hubを終了
        m_IsRun = false; // プロジェクト選択後、Hubを終了
    }
}

void HubManager::Window()
{
	RenderFullScreenUI();
	ShowSidebar();
	ShowMainContent();
}

void HubManager::RenderFullScreenUI()
{
    // ウィンドウフラグ
    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar;

    // ウィンドウサイズ = ディスプレイサイズ
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    if (ImGui::Begin("FullScreenUI", nullptr, window_flags))
    {
        // メニューバー
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                ImGui::MenuItem("Open Project");
                ImGui::MenuItem("Exit");
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        // DockSpace（中央のレイアウト分割領域）
        ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f),
            ImGuiDockNodeFlags_PassthruCentralNode |
            ImGuiDockNodeFlags_NoDockingInCentralNode);
    }
    ImGui::End();

    ImGui::PopStyleVar(2);
}

void HubManager::ShowSidebar()
{
    ImGui::Begin("Sidebar");

    ImGui::Text("Projects");

    static char newProjectName[256] = "";
    static bool showErrorPopup = false;
    static std::string errorMessage;

    ImGui::Text("New Project Name:");
    ImGui::InputText("##NewProject", newProjectName, sizeof(newProjectName));

    if (ImGui::Button("Create New Project"))
    {
        std::wstring name = ConvertString(newProjectName);
        if (!name.empty())
        {
            // ダイアログを開いて保存先フォルダを選択
            std::wstring projectPath = FileSystem::GameBuilder::SelectFolderDialog();
            if (!projectPath.empty())
            {
                bool created = FileSystem::CreateNewProjectFolder(name, projectPath);
                if (created)
                {
                    // デフォルトのシーンを作成
                    GameScene scene = m_pEngineCommand->GetGameCore()->GetSceneManager()->CreateDefaultScene();
                    // エディタにセット、ロード
                    m_pEngineCommand->GetEditorManager()->ChangeEditingScene(scene.GetName());
                    // プロジェクト名を保存
                    FileSystem::m_sProjectName = name;
                    FileSystem::m_sProjectFolderPath = projectPath + L"\\" + name;
                    // プロジェクトフォルダを作成
                    FileSystem::ScriptProject::GenerateSolutionAndProject();
                    // キャッシュに追加
                    FileSystem::g_CacheFile.projectNames.push_back(FileSystem::m_sProjectFolderPath);
                    // ブランチを取得
                    GetCurrentBranch();
                    // プロジェクトを保存
                    FileSystem::SaveProject(m_pEngineCommand->GetEditorManager(), m_pEngineCommand->GetGameCore()->GetSceneManager(), m_pEngineCommand->GetGameCore()->GetGameWorld(), m_pEngineCommand->GetGameCore()->GetECSManager(),m_pEngineCommand);
                    m_IsRun = false; // プロジェクト作成後、Hubを終了
                }
                else
                {
                    // エラー発生（すでに存在・作成失敗など）
                    errorMessage = "プロジェクト作成に失敗しました。同名のプロジェクトが存在するか、権限がありません。";
                    ImGui::OpenPopup("ErrorPopup");
                }
            }
        }
    }

    // エラーポップアップ
    if (ImGui::BeginPopupModal("ErrorPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextWrapped("%s", errorMessage.c_str());
        if (ImGui::Button("OK", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::Separator();

    // ディスクからプロジェクトを探す
    if(ImGui::Button(ConvertString(L"ディスクからプロジェクトを探す").c_str()))
    {
        // ダイアログを開いてプロジェクトフォルダを選択
        std::wstring folder = FileSystem::GameBuilder::SelectFolderDialog();
        if (!folder.empty())
        {
            // プロジェクトフォルダを読み込む
            FileSystem::LoadProjectFolder(folder, m_pEngineCommand);
            // プロジェクト名を保存
            std::filesystem::path projectName = std::filesystem::path(folder).filename();
            FileSystem::m_sProjectName = projectName;
            // ブランチを取得
            GetCurrentBranch();
            // ウィンドウのタイトルバーを変更
            SetWindowTextW(WinApp::GetHWND(), FileSystem::g_GameSettings.titleBar.c_str());
            // プロジェクト選択後、Hubを終了
            m_IsRun = false; // プロジェクト選択後、Hubを終了
            // プロジェクトパスを保存
            FileSystem::g_CacheFile.projectNames.push_back(folder);
        }
    }

    ImGui::Text("Select Project:");

    // プロジェクト一覧を取得
    std::vector<std::wstring> projects = FileSystem::g_CacheFile.projectNames;
    for(const auto& proj : projects)
    {
        // 存在しないプロジェクトはリストから削除
        if (!std::filesystem::exists(proj))
        {
            projects.erase(std::remove(projects.begin(), projects.end(), proj), projects.end());
            continue;
        }
        std::filesystem::path path(proj);
        std::string utf8Name = ConvertString(path.filename().wstring());
        if (ImGui::Selectable(utf8Name.c_str(), FileSystem::m_sProjectName == path.filename().wstring()))
        {
            // プロジェクトの読み込み
            FileSystem::LoadProjectFolder(path.wstring(), m_pEngineCommand);
            // プロジェクト名を保存
            FileSystem::m_sProjectName = path.filename().wstring();
            // ブランチを取得
            GetCurrentBranch();
            // ウィンドウのタイトルバーを変更
            SetWindowTextW(WinApp::GetHWND(), FileSystem::g_GameSettings.titleBar.c_str());
            // プロジェクト選択後、Hubを終了
            m_IsRun = false; // プロジェクト選択後、Hubを終了
        }
    }

  //  static std::vector<std::wstring> projects = FileSystem::GetProjectFolders();
  //  static int selectedIndex = -1;

  //  for (int i = 0; i < projects.size(); ++i)
  //  {
		//std::string utf8Name = ConvertString(projects[i]);
  //      if (ImGui::Selectable(utf8Name.c_str(), selectedIndex == i))
  //      {
  //          selectedIndex = i;
  //          std::wstring selectedProjectName = projects[i];
  //          // プロジェクトの読み込み
		//	FileSystem::LoadProjectFolder(selectedProjectName, m_pEngineCommand);
		//	// プロジェクトのパスを保存
		//	FileSystem::ScriptProject::LoadProjectPath(selectedProjectName);
		//	// ブランチを取得
  //          GetCurrentBranch();
		//	// プロジェクト選択後、Hubを終了
		//	m_IsRun = false; // プロジェクト選択後、Hubを終了
  //      }
  //  }

    ImGui::End();
}


void HubManager::ShowMainContent()
{
    ImGui::Begin("MainView");
    ImGui::Text("Welcome to ChoHub!");
    // 中央の画面など
    ImGui::End();
}

void HubManager::GetCurrentBranch()
{
	std::string gitFolder = ConvertString(FileSystem::m_sProjectFolderPath) + "/.git";
	std::filesystem::path gitPath(gitFolder);
	m_GitHeadPath = gitPath / "HEAD";
    m_LastBranch = ReadCurrentBranch();
}

bool HubManager::CheckBranchChanged()
{
	// ブランチが変更されたかどうかを確認
	// .gitが存在しない場合は常に探して見つかったらそれをlastBranchにする
    if (m_LastBranch.empty())
    {
		m_LastBranch = ReadCurrentBranch();
        return false;
    }
    std::string currentBranch = ReadCurrentBranch();
    if (currentBranch != m_LastBranch)
    {
        m_LastBranch = currentBranch;
        return true;
    }
    return false;
}

void HubManager::ReloadProject()
{
    // プロジェクトの読み込み
    FileSystem::LoadProjectFolder(FileSystem::m_sProjectName, m_pEngineCommand);
    // ブランチを取得
    GetCurrentBranch();
    // プロジェクト選択後、Hubを終了
    m_IsRun = false; // プロジェクト選択後、Hubを終了
}
