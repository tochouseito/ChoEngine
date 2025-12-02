#pragma once
#include "Editor/BaseEditor/BaseEditor.h"
class ShaderEditor : public BaseEditor
{
public:
	ShaderEditor(EditorManager* editorManager) :
		BaseEditor(editorManager)
	{
	}
	~ShaderEditor()
	{
	}
	void Initialize() override;
	void Update() override;
	void Window() override;
};

