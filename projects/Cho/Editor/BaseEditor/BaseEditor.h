#pragma once
#include "Core/Utility/ImGuiEx.h"
class EngineCommand;
class EditorManager;
class BaseEditor
{
public:
	BaseEditor(EditorManager* editorManager);
	~BaseEditor()
	{
	}
	virtual void Initialize() = 0;
	virtual void Update() = 0;
	virtual void Window() = 0;
protected:
	EditorManager* m_EditorManager = nullptr;
	EngineCommand* m_EngineCommand = nullptr;
};

