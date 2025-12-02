#pragma once
/*--------------------------------------------
エンジン基底クラス
--------------------------------------------*/

#include "Config/Config.h"

// C++
#include <memory>

enum class RuntimeMode
{
	Game,
	Editor,
};

class Engine
{
public:// method
	// コンストラクタ
	Engine(RuntimeMode mode) : runtimeMode(mode)
	{
	}

	// デストラクタ
	virtual ~Engine() = default;

	virtual void Initialize() {}
	virtual void Finalize() {}
	virtual void Operation() {}
	virtual void CrashHandlerEntry() {}
	virtual RuntimeMode GetRuntimeMode() const
	{
		return runtimeMode;
	}
protected:
	RuntimeMode runtimeMode;
	theatria::Config config;
};

