#pragma once
#include "Editor/BaseEditor/BaseEditor.h"

// 前方宣言
class DebugCamera;

class Manipulate : public BaseEditor
{
	friend class SceneView;
public:
	Manipulate(EditorManager* editorManager,DebugCamera* dCamera) :
		BaseEditor(editorManager), m_dCamera(dCamera)
	{

	}
	~Manipulate()
	{

	}
	// BaseEditor を介して継承されました
	void Initialize() override;
	void Update() override;
	void Window() override;
private:
	void SetContent(float2 pos, float2 size)
	{
		m_ContentPos = pos;
		m_ContentSize = size;
	}

	DebugCamera* m_dCamera = nullptr;
	float2 m_ContentPos{};
	float2 m_ContentSize{};
};

