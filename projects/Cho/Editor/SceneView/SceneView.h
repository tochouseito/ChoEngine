#pragma once
#include "Editor/BaseEditor/BaseEditor.h"
class DebugCamera;
class Manipulate;
class SceneView : public BaseEditor
{
public:
	SceneView(EditorManager* editorManager,DebugCamera* debugCamera, Manipulate* manipulate) :
		BaseEditor(editorManager), m_DebugCamera(debugCamera), m_Manipulate(manipulate)
	{
	}
	~SceneView()
	{
	}
	void Initialize() override;
	void Update() override;
	void Window() override;
private:
	DebugCamera* m_DebugCamera = nullptr;
	Manipulate* m_Manipulate = nullptr;
};

