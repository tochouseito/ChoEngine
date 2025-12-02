#pragma once
#include "Editor/BaseEditor/BaseEditor.h"
class Toolbar : public BaseEditor
{
public:
	Toolbar(EditorManager* editorManager) :
		BaseEditor(editorManager)
	{
	}
	~Toolbar()
	{
	}
	void Initialize() override;
	void Update() override;
	void Window() override;
};

