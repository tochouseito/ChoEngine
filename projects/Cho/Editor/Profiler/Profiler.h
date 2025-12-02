#pragma once
#include "Editor/BaseEditor/BaseEditor.h"
class Profiler : public BaseEditor
{
public:
	Profiler(EditorManager* editorManager) :
		BaseEditor(editorManager)
	{
	}
	~Profiler()
	{
	}
	void Initialize() override;
	void Update() override;
	void Window() override;
};

