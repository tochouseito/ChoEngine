#pragma once
#include "Editor/BaseEditor/BaseEditor.h"
class AnimationEditor : public BaseEditor
{
public:
	AnimationEditor(EditorManager* editorManager) :
		BaseEditor(editorManager)
	{
	}
	~AnimationEditor()
	{
	}
	void Initialize() override;
	void Update() override;
	void Window() override;
};

