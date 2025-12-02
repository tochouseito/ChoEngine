#pragma once
#include "Editor/BaseEditor/BaseEditor.h"
class PipelineEditor : public BaseEditor
{
public:
	PipelineEditor(EditorManager* editorManager) :
		BaseEditor(editorManager)
	{
	}
	~PipelineEditor()
	{
	}
	void Initialize() override;
	void Update() override;
	void Window() override;
};

