#pragma once

#include"AbstractSceneFactory.h"
#include"Scene/BaseScene/BaseScene.h"

class SceneFactory :public AbstractSceneFactory {
public:
	/// <summary>
	/// シーン生成
	/// </summary>
	/// <param name="sceneName"></param>
	/// <returns></returns>
	BaseScene* CreateScene(const std::string& sceneName) override;

private:

};
 
