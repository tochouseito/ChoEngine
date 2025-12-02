#include "pch.h"
#include "PlatformLayer.h"

PlatformLayer::PlatformLayer()
{
	m_Timer = std::make_unique<Timer>();
	m_InputManager = std::make_unique<InputManager>();
	Initialize();
}

PlatformLayer::~PlatformLayer()
{
}

void PlatformLayer::Initialize()
{
}

void PlatformLayer::Finalize()
{

}

void PlatformLayer::Update()
{
	m_Timer->Update();
	m_InputManager->Update();
}

void PlatformLayer::StartFrame()
{
	m_Timer->Start();
}

void PlatformLayer::EndFrame()
{
	m_Timer->End();
}
