#pragma once
#include "Editor/BaseEditor/BaseEditor.h"
class Timeline : public BaseEditor
{
public:
	Timeline(EditorManager* editorManager) :
		BaseEditor(editorManager)
	{
	}
	~Timeline()
	{
	}
	void Initialize() override;
	void Update() override;
	void Window() override;
};

