#include "pch.h"
#include "CoreSystem.h"

CoreSystem::CoreSystem()
{
	Initialize();
}

CoreSystem::~CoreSystem()
{
}

void CoreSystem::Initialize()
{
	// スレッドマネージャー
    threadManager = std::make_unique<ThreadManager>();
}

void CoreSystem::Finalize()
{
	threadManager->StopAllThreads();
}
