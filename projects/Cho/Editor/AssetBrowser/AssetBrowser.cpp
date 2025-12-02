#include "pch.h"
#include "AssetBrowser.h"
#include "Editor/EditorManager/EditorManager.h"
#include "Core/Utility/FontCode.h"
#include "OS/Windows/WinApp/WinApp.h"

void AssetBrowser::Initialize()
{

}

void AssetBrowser::Update()
{
	Window();
}

void AssetBrowser::Window()
{
	// 移動を無効にするフラグ
	//ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoMove;
	ImGui::Begin("AssetBrowser", nullptr);
    if (ImGui::BeginTable("TreeTable", 1, ImGuiTableFlags_BordersInnerV))
    {
        // 左：フォルダー階層
        FolderTree(theatria::FileSystem::g_ProjectFiles);
		ImGui::EndTable();
    }
	ImGui::End();
	// ファイルの表示
	ImGui::Begin("FileGrid");
    // 右：ファイル表示
    FileGrid(theatria::FileSystem::g_ProjectFiles);
    ImGui::End();
}

void AssetBrowser::FolderTree(FolderNode& node)
{
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    std::string label = node.folderPath.filename().string();
    if (label.empty()) { label = "Assets"; }
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_DrawLinesFull;
    bool opened = ImGui::TreeNodeEx(label.c_str(),flags);
    if (ImGui::IsItemClicked())
    {
        m_SelectedFolder = node.folderPath;
    }
    if (opened)
    {
        for (auto& child : node.children)
        {
            FolderTree(child);
        }
        ImGui::TreePop();
    }
}

void AssetBrowser::FileGrid(FolderNode& root)
{
    FolderNode* target = theatria::FileSystem::FindFolderNodeByPath(root, m_SelectedFolder);
    if (!target) return;

	// ウィンドウからファイル名のドロップがあった場合
    if (WinApp::IsDropFiles())
    {
		for (const auto& filePath : WinApp::GetDropFiles())
		{
			// ドロップされたファイルを処理する
			std::filesystem::path dropPath = filePath;
			theatria::FileSystem::AddFile(dropPath, *target, m_EditorManager->GetEngineCommand());
		}
    }
    
    constexpr float iconSize = 64.0f;
    constexpr float padding = 8.0f;
    float cellSize = iconSize + padding;
    int columnCount = static_cast<int>(ImGui::GetContentRegionAvail().x / cellSize);
    if (columnCount < 1) columnCount = 1;

    ImGui::Columns(columnCount, nullptr, false);

    for (const auto& filePath : target->files)
    {
        std::wstring wname = filePath.stem().wstring();
        std::wstring wext = filePath.extension().wstring();
		std::string name = ConvertString(wname);
		std::string ext = ConvertString(wext);
        std::string icon = GetIconForExtension(ext, false);
		std::string uniqueId = icon + "##" + name;

        // 画像ファイルのみドラッグ＆ドロップを有効にする
        if (ext == ".png" || ext == ".jpg")
        {
            // 画像を表示
            ImTextureID textureId = (ImTextureID)m_EngineCommand->GetTextureHandle(wname).ptr;
            ImGui::ImageButton(uniqueId.c_str(), textureId, ImVec2(iconSize, iconSize));
            if (ImGui::BeginDragDropSource())
            {
                const char* fileName = name.c_str();
                ImGui::SetDragDropPayload("Texture", fileName, strlen(fileName) + 1);
                ImGui::EndDragDropSource();
            }
        } else if (ext == ".fbx" || ext ==".obj"||ext ==".gltf")
        {
            ImGui::Button(uniqueId.c_str(), ImVec2(iconSize, iconSize));
            if (ImGui::BeginDragDropSource())
            {
                const char* fileName = name.c_str();
                ImGui::SetDragDropPayload("ModelData", fileName, strlen(fileName) + 1);
                ImGui::EndDragDropSource();
            }
        }else
        {
            ImGui::Button(uniqueId.c_str(), ImVec2(iconSize, iconSize));
        }

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
			// ファイルを開く処理
			//std::string path = filePath.string();
			//ShellExecuteA(nullptr, "open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        }

        ImGui::TextWrapped("%s", name.c_str());
        ImGui::NextColumn();
    }

    ImGui::Columns(1);
}

std::string AssetBrowser::GetIconForExtension(const std::string& ext, bool isDirectory)
{
    if (isDirectory) return ICON_MATERIAL_FOLDER;
    if (ext == ".obj" || ext == ".gltf") return ICON_MATERIAL_MODEL;
    if (ext == ".png" || ext == ".jpg") return ICON_MATERIAL_IMAGE;
    if (ext == ".wav" || ext == ".mp3") return ICON_MATERIAL_AUDIO;
    if (ext == ".effect") return ICON_MATERIAL_EFFECT;
    if (ext == ".scene") return ICON_MATERIAL_SCENE;
    if (ext == ".cpp" || ext == ".h") return ICON_MATERIAL_SCRIPT;
    return std::string(ICON_MATERIAL_FILE);
}
