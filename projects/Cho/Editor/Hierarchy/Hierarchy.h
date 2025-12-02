#pragma once
#include "Editor/BaseEditor/BaseEditor.h"
class Hierarchy : public BaseEditor
{
public:
	Hierarchy(EditorManager* editorManager) :
		BaseEditor(editorManager)
	{
	}
	~Hierarchy()
	{
	}
	void Initialize() override;
	void Update() override;
	void Window() override;
};

