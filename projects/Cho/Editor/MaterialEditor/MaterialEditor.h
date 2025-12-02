#pragma once
#include "Editor/BaseEditor/BaseEditor.h"

class MaterialEditor : public BaseEditor
{
public:
	MaterialEditor(EditorManager* editorManager);
	~MaterialEditor();
	void Initialize() override;
	void Update() override;
	void Window() override;
private:
    void ImGuiEx_BeginColumn();
    void ImGuiEx_NextColumn();
    void ImGuiEx_EndColumn();

	void BasicNodeExample();

	struct Impl;
	std::unique_ptr<Impl> impl;
	
	bool m_FirstFrame = true; // 最初のフレームかどうか
	int m_NextLinkId = 100; // リンクIDのカウンター

	int m_NextId = 1;
	const int m_PinIconSize = 24;
	const float m_TouchTime = 1.0f;
	bool m_ShowOrdinals = false; // オーディナル表示のフラグ
};

