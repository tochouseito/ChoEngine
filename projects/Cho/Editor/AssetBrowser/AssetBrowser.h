#pragma once
#include "Editor/BaseEditor/BaseEditor.h"
#include "Platform/FileSystem/FileSystem.h"
class AssetBrowser : public BaseEditor
{
public:
	AssetBrowser(EditorManager* editorManager) :
		BaseEditor(editorManager)
	{
	}
	~AssetBrowser()
	{
	}
	void Initialize() override;
	void Update() override;
	void Window() override;
private:
	void FolderTree(FolderNode& node);
    void FileGrid(FolderNode& root);
	std::string GetIconForExtension(const std::string& ext, bool isDirectory);

	// 選択中のフォルダ
	std::filesystem::path m_SelectedFolder;
};

