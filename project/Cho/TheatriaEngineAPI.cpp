#include "pch.h"
#define ENGINECREATE_FUNCTION
#define USE_THEATRIAENGINE_SCRIPT
#include "TheatriaEngineAPI.h"
#include "Main/TheatriaEngine.h"
#include "EngineCommand/EngineCommands.h"

// メンバ定義
namespace theatriaSystem
{
	SceneManagerAPI sceneManager;
	struct AudioAPI::ImplAudioAPI
	{
		ImplAudioAPI() = default;
		~ImplAudioAPI() = default;
		std::vector<SoundData> soundDatas;
	};
	AudioAPI::AudioAPI()
	{
		implAudioAPI = std::make_unique<ImplAudioAPI>();
	}
	AudioAPI testAudio;
	THEATRIA_API void AudioAPI::AddSource(const std::string& name)
	{
		for(const auto& src : implAudioAPI->soundDatas)
		{
			// 既に追加されているものは追加しない
			if(src.name == name) return;
		}
		ResourceManager* resourceManager = g_Engine->GetEngineCommand()->GetResourceManager();
		if (!resourceManager) { return; }
		std::unordered_map<std::string, uint32_t> audioNameMap = resourceManager->GetAudioManager()->GetSoundDataToName();
		for(const auto& pair : audioNameMap)
		{
			if (pair.first == name)
			{
				SoundData soundData = resourceManager->GetAudioManager()->CreateSoundData(name);
				if (implAudioAPI)
				{
					implAudioAPI->soundDatas.push_back(soundData);
				}
				break;
			}
		}
	}
	THEATRIA_API void AudioAPI::Play(const std::string& name, bool isLoop)
	{
		for(auto& src : implAudioAPI->soundDatas)
		{
			if(src.name == name)
			{
				ResourceManager* resourceManager = g_Engine->GetEngineCommand()->GetResourceManager();
				if (!resourceManager) { return; }
				resourceManager->GetAudioManager()->SoundPlayWave(src, isLoop);
				break;
			}
		}
	}
	THEATRIA_API void AudioAPI::Stop(const std::string& name)
	{
		name;
	}
	THEATRIA_API bool AudioAPI::IsPlaying(const std::string& name)
	{
		name;
		return false;
	}
	THEATRIA_API void AudioAPI::SetVolume(const std::string& name, float volume)
	{
		for (auto& src : implAudioAPI->soundDatas)
		{
			if (src.name == name)
			{
				ResourceManager* resourceManager = g_Engine->GetEngineCommand()->GetResourceManager();
				if (!resourceManager) { return; }
				src.currentVolume = volume;
				resourceManager->GetAudioManager()->SetVolume(src, volume);
				break;
			}
		}
	}
}

THEATRIA_API Engine* theatria::CreateEngine(RuntimeMode mode)
{
    return new TheatriaEngine(mode);
}

THEATRIA_API void theatria::DestroyEngine(Engine* engine)
{
	delete engine;
}

THEATRIA_API void theatria::SetEngine(Engine* engine)
{
	g_Engine = static_cast<TheatriaEngine*>(engine);
}

THEATRIA_API bool theatriaSystem::SaveGameParameter(const std::wstring& filePath, const std::string& group, const std::string& item, const std::string& dataName, const GameParameterVariant& value)
{
	return theatria::FileSystem::SaveGameParameter(filePath, group, item, dataName, value);
}

THEATRIA_API bool theatriaSystem::LoadGameParameter(const std::wstring& filePath, const std::string& group, const std::string& item, const std::string& dataName, GameParameterVariant& outValue)
{
	return theatria::FileSystem::LoadGameParameter(filePath, group, item, dataName, outValue);
}

// ゲームオブジェクト取得
THEATRIA_API GameObject* theatriaSystem::FindGameObjectByName(const std::wstring& name)
{
	GameCore* gameCore = g_Engine->GetEngineCommand()->GetGameCore();
	GameObject* result = gameCore->GetGameWorld()->GetGameObject(name);
	return result;
}

THEATRIA_API GameObject* theatriaSystem::CloneGameObject(const GameObject* srcObj, float3 generatePosition)
{
	EngineCommand* engineCommand = g_Engine->GetEngineCommand();
	std::unique_ptr<CloneObjectCommand> command = std::make_unique<CloneObjectCommand>(srcObj->GetHandle());
	command->Execute(engineCommand);
	GameObject* newObject = engineCommand->GetGameCore()->GetGameWorld()->GetGameObject(command->GetDstHandle());
	// CurrendSceneのコピー
	newObject->SetCurrentSceneName(srcObj->GetCurrentSceneName());
	// Componentを取得
	TransformComponent* transform = engineCommand->GetGameCore()->GetECSManager()->GetComponent<TransformComponent>(newObject->GetHandle().entity);
	if (newObject->GetType() == ObjectType::ParticleSystem)
	{
		transform->scale.Zero();
	}
	// 初期位置を設定
	if (transform)
	{
		transform->position = generatePosition;
	}
	// Rigidbodyがあれば初期位置を指定
	Rigidbody3DComponent* rb3d = engineCommand->GetGameCore()->GetECSManager()->GetComponent<Rigidbody3DComponent>(newObject->GetHandle().entity);
	if(transform && rb3d && rb3d->runtimeBody)
	{
		rb3d->runtimeBody->SetTransform(transform->position);
	}
	return newObject;
}

// ゲームオブジェクト削除
THEATRIA_API void theatriaSystem::DestroyGameObject(GameObject* obj)
{
	EngineCommand* engineCommand = g_Engine->GetEngineCommand();
	if (!obj) { return; }
	std::unique_ptr<DeleteObjectCommand> command = std::make_unique<DeleteObjectCommand>(obj->GetHandle());
	command->Execute(engineCommand);
}

THEATRIA_API float theatriaSystem::DeltaTime()
{
	return Timer::GetDeltaTime();
}

THEATRIA_API uint32_t theatriaSystem::ScreenWidth()
{
	EngineCommand* engineCommand = g_Engine->GetEngineCommand();
	return static_cast<uint32_t>(engineCommand->GetGraphicsEngine()->GetResolutionWidth());
}

THEATRIA_API uint32_t theatriaSystem::ScreenHeight()
{
	EngineCommand* engineCommand = g_Engine->GetEngineCommand();
	return static_cast<uint32_t>(engineCommand->GetGraphicsEngine()->GetResolutionHeight());
}

THEATRIA_API Marionnette* theatriaSystem::GetMarionnettePtr(const std::wstring& name)
{
	GameObject* obj = FindGameObjectByName(name);
	if (!obj) { return nullptr; }
	ScriptComponent* script = g_Engine->GetEngineCommand()->GetGameCore()->GetECSManager()->GetComponent<ScriptComponent>(obj->GetHandle().entity);
	if (!script) { return nullptr; }
	if (script->instance)
	{
		return script->instance;
	}
	else
	{
		//Log::Write(LogLevel::Error, L"Marionnette instance is nullptr for object: " + name);
		return nullptr;
	}
}

THEATRIA_API theatriaSystem::Emitter theatriaSystem::GetEmitterComponent(GameObject& obj)
{
    return Emitter(obj.GetHandle().entity, g_Engine->GetEngineCommand()->GetGameCore()->GetECSManager());
}

std::vector<std::vector<std::string>> theatriaSystem::LoadCSV(const std::string& filePath)
{
	return theatria::FileSystem::LoadCSV(filePath);
}

std::vector<std::vector<int>> theatriaSystem::LoadCSV_Int(const std::string& filePath)
{
	return theatria::FileSystem::LoadCSV_Int(filePath);
}

void theatriaSystem::SceneManagerAPI::LoadScene(const std::wstring& sceneName)
{
	g_Engine->GetEngineCommand()->GetGameCore()->GetSceneManager()->SetLoadingSceneName(sceneName);
}

void theatriaSystem::SceneManagerAPI::UnloadScene(const std::wstring& sceneName)
{
	sceneName;
}

void theatriaSystem::SceneManagerAPI::ChangeMainScene(const std::wstring& sceneName)
{
	sceneName;
}

bool theatriaSystem::Input::PushKey(uint8_t keyNumber)
{
	return g_Engine->GetEngineCommand()->GetInputManager()->PushKey(keyNumber);
}

bool theatriaSystem::Input::TriggerKey(uint8_t keyNumber)
{
	return g_Engine->GetEngineCommand()->GetInputManager()->TriggerKey(keyNumber);
}

const DIMOUSESTATE2& theatriaSystem::Input::GetAllMouse()
{
	return g_Engine->GetEngineCommand()->GetInputManager()->GetAllMouse();
}

MouseMove theatriaSystem::Input::GetMouseMove()
{
	return g_Engine->GetEngineCommand()->GetInputManager()->GetMouseMove();
}

bool theatriaSystem::Input::IsPressMouse(int32_t mouseNumber)
{
	return g_Engine->GetEngineCommand()->GetInputManager()->IsPressMouse(mouseNumber);
}

bool theatriaSystem::Input::IsTriggerMouse(int32_t mouseNumber)
{
	return g_Engine->GetEngineCommand()->GetInputManager()->IsTriggerMouse(mouseNumber);
}

const float2& theatriaSystem::Input::GetMouseWindowPosition()
{
	return g_Engine->GetEngineCommand()->GetInputManager()->GetMouseWindowPosition();
}

float2 theatriaSystem::Input::GetMouseScreenPosition()
{
	return g_Engine->GetEngineCommand()->GetInputManager()->GetMouseScreenPosition();
}

bool theatriaSystem::Input::GetJoystickState(int32_t stickNo, XINPUT_STATE& out)
{
	return g_Engine->GetEngineCommand()->GetInputManager()->GetJoystickState(stickNo, out);
}

bool GetJoystickStatePrevious(int32_t stickNo, XINPUT_STATE& out)
{
	return g_Engine->GetEngineCommand()->GetInputManager()->GetJoystickStatePrevious(stickNo, out);
}

void SetJoystickDeadZone(int32_t stickNo, int32_t deadZoneL, int32_t deadZoneR)
{
	g_Engine->GetEngineCommand()->GetInputManager()->SetJoystickDeadZone(stickNo, deadZoneL, deadZoneR);
}

size_t GetNumberOfJoysticks()
{
	return g_Engine->GetEngineCommand()->GetInputManager()->GetNumberOfJoysticks();
}

bool IsTriggerPadButton(PadButton button, int32_t stickNo)
{
	return g_Engine->GetEngineCommand()->GetInputManager()->IsTriggerPadButton(button, stickNo);
}

bool IsPressPadButton(PadButton button, int32_t stickNo)
{
	return g_Engine->GetEngineCommand()->GetInputManager()->IsPressPadButton(button, stickNo);
}

float2 GetStickValue(LR padStick, int32_t stickNo)
{
	return g_Engine->GetEngineCommand()->GetInputManager()->GetStickValue(padStick, stickNo);
}

float GetLRTrigger(LR LorR, int32_t stickNo)
{
	return g_Engine->GetEngineCommand()->GetInputManager()->GetLRTrigger(LorR, stickNo);
}
