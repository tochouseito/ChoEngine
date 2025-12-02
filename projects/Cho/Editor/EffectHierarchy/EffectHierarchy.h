#pragma once
#include "Editor/BaseEditor/BaseEditor.h"
class EffectHierarchy : public BaseEditor
{
public:
	EffectHierarchy(EditorManager* editorManager) :
		BaseEditor(editorManager)
	{
	}
	~EffectHierarchy()
	{
	}
	void Initialize() override;
	void Update() override;
	void Window() override;
};

