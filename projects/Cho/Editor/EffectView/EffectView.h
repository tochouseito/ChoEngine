#pragma once
#include "Editor/BaseEditor/BaseEditor.h"
class DebugCamera;
class EffectView : public BaseEditor
{
public:
	EffectView(EditorManager* editorManager,DebugCamera* debugCamera) :
		m_DebugCamera(debugCamera), BaseEditor(editorManager)
	{
	}
	~EffectView()
	{
	}
	void Initialize() override;
	void Update() override;
	void Window() override;
private:
	DebugCamera* m_DebugCamera = nullptr;
};

