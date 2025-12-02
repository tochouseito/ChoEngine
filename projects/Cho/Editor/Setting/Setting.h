#pragma once
#include "Editor/BaseEditor/BaseEditor.h"
class Setting : public BaseEditor
{
public:
	Setting(EditorManager* editorManager) :
		BaseEditor(editorManager)
	{
	}
	~Setting()
	{
	}
	void Initialize() override;
	void Update() override;
	void Window() override;
};

