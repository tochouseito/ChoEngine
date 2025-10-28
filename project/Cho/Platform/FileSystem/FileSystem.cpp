#include "pch.h"
#include "FileSystem.h"
#include "GameCore/GameCore.h"
#include "Resources/ResourceManager/ResourceManager.h"
#include "Editor/EditorManager/EditorManager.h"
#include "EngineCommand/EngineCommand.h"
#include "EngineCommand/EngineCommands.h"
#include <cstdlib>
#include "Core/ChoLog/ChoLog.h"
#include <windows.h>
#include <winsock.h>
#include <wingdi.h>
#include <shellapi.h>
#include <shobjidl.h> 
#include <ole2.h>
#include <DbgHelp.h>
#include <comdef.h>
#include <atlbase.h>
#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
using namespace theatria;

std::wstring theatria::FileSystem::m_sProjectName = L"";
std::wstring theatria::FileSystem::m_sProjectFolderPath = L"";
// GUID 生成
std::string theatria::FileSystem::ScriptProject::m_SlnGUID = GenerateGUID();
std::string theatria::FileSystem::ScriptProject::m_ProjGUID = GenerateGUID();
std::string theatria::FileSystem::ScriptProject::m_SlnPath = "";
std::string theatria::FileSystem::ScriptProject::m_ProjPath = "";
HMODULE theatria::FileSystem::ScriptProject::m_DllHandle = nullptr;
DWORD64 theatria::FileSystem::ScriptProject::m_PDBBaseAddress = 0;
HANDLE theatria::FileSystem::ScriptProject::m_ReadPipe = nullptr; // 読み取り用パイプ
HANDLE theatria::FileSystem::ScriptProject::m_WritePipe = nullptr; // 書き込み用パイプ
bool theatria::FileSystem::ScriptProject::m_IsAttached = false;

// スクリプトプロジェクトの自動保存、ビルド
static HRESULT InvokeByName(IDispatch* pDisp, LPCOLESTR name, VARIANT* args, UINT cArgs)
{
    DISPID dispid;
    HRESULT hr = pDisp->GetIDsOfNames(IID_NULL, const_cast<LPOLESTR*>(&name), 1, LOCALE_USER_DEFAULT, &dispid);
    if (FAILED(hr)) return hr;

    // 引数は逆順で渡す（rgvarg[0] が最後の引数）
    DISPPARAMS dp = {};
    dp.cArgs = cArgs;
    dp.rgvarg = args;
    return pDisp->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &dp, nullptr, nullptr, nullptr);
}

// プロジェクトフォルダを探す
std::optional<std::filesystem::path> theatria::FileSystem::FindOrCreateGameProjects()
{
    namespace fs = std::filesystem;
    fs::path execPath = fs::current_path();
    fs::path gameProjectsPath = execPath / "GameProjects";

    if (!fs::exists(gameProjectsPath))
    {
        if (!fs::create_directory(gameProjectsPath))
        {
            return std::nullopt;
        }
    }

    return gameProjectsPath;
}

// プロジェクトフォルダを取得
std::vector<std::wstring> theatria::FileSystem::GetProjectFolders()
{
    std::vector<std::wstring> result;

    auto rootOpt = FindOrCreateGameProjects();
    if (!rootOpt) return result;

    for (const auto& entry : std::filesystem::directory_iterator(*rootOpt))
    {
        if (entry.is_directory())
        {
            result.push_back(entry.path().filename().wstring());
        }
    }

    return result;
}

// 新しいプロジェクトを作成
bool theatria::FileSystem::CreateNewProjectFolder(const std::wstring& projectName, const std::wstring& projectPath)
{
    std::filesystem::path newProjectPath = projectPath + L"/" + projectName;

    if (std::filesystem::exists(newProjectPath))
    {
        // すでに同名のプロジェクトがある
        return false;
    }
    if (!std::filesystem::create_directory(newProjectPath))
    {
		return false; // プロジェクトフォルダの作成に失敗
    }
	// Assets フォルダを作成
	std::filesystem::path assetsPath = newProjectPath / "Assets";
    if(!std::filesystem::create_directory(assetsPath))
    {
        return false; // Assets フォルダの作成に失敗
	}

	// ProjectSettings フォルダを作成
	std::filesystem::path projectSettingsPath = newProjectPath / "ProjectSettings";
    if (!std::filesystem::create_directory(projectSettingsPath))
    {
        return false; // ProjectSettings フォルダの作成に失敗
    }

    //// Packages フォルダを作成
    //std::filesystem::path packagesPath = newProjectPath / "Packages";
    //if (!std::filesystem::create_directory(packagesPath))
    //{
    //    return false; // Packages フォルダの作成に失敗
    //}

    //// Packagesの中にlib,includeフォルダを作成
    //std::filesystem::path libPath = packagesPath / "lib";
    //if (!std::filesystem::create_directory(libPath))
    //{
    //    return false; // libフォルダの作成に失敗
    //}
    //std::filesystem::path includePath = packagesPath / "include";
    //if (!std::filesystem::create_directory(includePath))
    //{
    //    return false; // includeフォルダの作成に失敗
    //}

    //// libフォルダにTheatriaEngine_GameRuntime.lib,.exp,.dllを"_GameRuntime"を外してコピー
    //std::filesystem::path engineLibPath = std::filesystem::current_path() / "TheatriaEngine_GameRuntime.lib";
    //std::filesystem::path engineExpPath = std::filesystem::current_path() / "TheatriaEngine_GameRuntime.exp";
    //std::filesystem::path engineDllPath = std::filesystem::current_path() / "TheatriaEngine_GameRuntime.dll";
    //if (std::filesystem::exists(engineLibPath))
    //{
    //    std::filesystem::copy_file(engineLibPath, libPath / "TheatriaEngine.lib");
    //}
    //if (std::filesystem::exists(engineExpPath))
    //{
    //    std::filesystem::copy_file(engineExpPath, libPath / "TheatriaEngine.exp");
    //}
    //if (std::filesystem::exists(engineDllPath))
    //{
    //    std::filesystem::copy_file(engineDllPath, libPath / "TheatriaEngine.dll");
    //}
    //// ChoMath.libをコピー
    //std::filesystem::path mathLibPath = std::filesystem::current_path() / "ChoMath.lib";
    //if (std::filesystem::exists(mathLibPath))
    //{
    //    std::filesystem::copy_file(mathLibPath, libPath / "ChoMath.lib");
    //}
    //// includeフォルダにCho,ChoMath,PhysicsEngineフォルダを中身ごとコピー
    //std::filesystem::path choIncludePath = std::filesystem::current_path() / "Cho";
    //if (std::filesystem::exists(choIncludePath))
    //{
    //    std::filesystem::copy(choIncludePath, includePath / "Cho", std::filesystem::copy_options::recursive);
    //}
    //std::filesystem::path choMathIncludePath = std::filesystem::current_path() / "ChoMath";
    //if (std::filesystem::exists(choMathIncludePath))
    //{
    //    std::filesystem::copy(choMathIncludePath, includePath / "ChoMath", std::filesystem::copy_options::recursive);
    //}
    //std::filesystem::path physicsIncludePath = std::filesystem::current_path() / "PhysicsEngine";
    //if (std::filesystem::exists(physicsIncludePath))
    //{
    //    std::filesystem::copy(physicsIncludePath, includePath / "PhysicsEngine", std::filesystem::copy_options::recursive);
    //}

	return true; // プロジェクトフォルダの作成に成功
}

// プロジェクトファイルを保存
bool theatria::FileSystem::SaveProjectFile(const std::wstring& projectName, const std::vector<std::wstring>& sceneFiles)
{
    std::filesystem::path path = std::filesystem::path(projectName) / "ChoProject.json";

    nlohmann::ordered_json j;
    j["fileType"] = "ChoProject";
    j["name"] = std::filesystem::path(projectName).filename().string(); // プロジェクト名
    j["version"] = "1.0.0";

    // wstring → string 変換（UTF-8）
    std::vector<std::string> sceneNames;
    for (const auto& ws : sceneFiles)
    {
        sceneNames.emplace_back(std::filesystem::path(ws).string()); // or std::wstring_convert
    }

    j["scenes"] = sceneNames;

    try
    {
        std::ofstream file(path.string());
        if (!file.is_open())
        {
            return false;
        }
        file << j.dump(4); // 整形
        file.close();
        return true;
    }
    catch (...)
    {
        return false;
    }
}

// プロジェクトファイルを読み込む
std::optional<theatria::ProjectInfo> theatria::FileSystem::LoadProjectFile(const std::wstring& projectName)
{
    std::filesystem::path path = std::filesystem::path(projectName) / "ChoProject.json";

    try
    {
        std::ifstream file(path.string());
        if (!file.is_open())
        {
            return std::nullopt;
        }

        nlohmann::json j;
        file >> j;

        if (j.contains("fileType") && j["fileType"] != "ChoProject")
        {
            return std::nullopt; // ファイル種別違い
        }

        theatria::ProjectInfo info;
        info.name = j.value("name", "");
        info.version = j.value("version", "");

        for (const auto& sceneName : j["scenes"])
        {
            std::string s = sceneName.get<std::string>();
            info.scenes.push_back(std::filesystem::path(s).wstring());
        }

        return info;
    }
    catch (...)
    {
        return std::nullopt;
    }
}

// エンジン設定ファイルを保存
bool theatria::FileSystem::SaveEngineConfig(const std::wstring& projectName, const EngineConfigInfo& config)
{
    std::filesystem::path path = std::filesystem::path(projectName) / "EngineConfig.json";

    nlohmann::ordered_json j;
    j["fileType"] = "EngineConfig";
    j["renderer"] = config.renderer;
    j["window"] = {
        { "width", config.window.width },
        { "height", config.window.height },
        { "fullscreen", config.window.fullscreen }
    };
    j["vsync"] = config.vsync;

    try
    {
        std::ofstream file(path.string());
        if (!file.is_open()) return false;

        file << j.dump(4);
        file.close();
        return true;
    }
    catch (...)
    {
        return false;
    }
}

// エンジン設定ファイルを読み込む
std::optional<theatria::EngineConfigInfo> theatria::FileSystem::LoadEngineConfig(const std::wstring& projectName)
{
    std::filesystem::path path = std::filesystem::path(projectName) / "EngineConfig.json";

    try
    {
        std::ifstream file(path.string());
        if (!file.is_open()) return std::nullopt;

        nlohmann::json j;
        file >> j;

        if (j.contains("fileType") && j["fileType"] != "EngineConfig")
        {
            return std::nullopt;
        }

        EngineConfigInfo config;
        config.renderer = j.value("renderer", "DirectX12");

        if (j.contains("window"))
        {
            const auto& win = j["window"];
            config.window.width = win.value("width", 1280);
            config.window.height = win.value("height", 720);
            config.window.fullscreen = win.value("fullscreen", false);
        }

        config.vsync = j.value("vsync", true);
        return config;
    }
    catch (...)
    {
        return std::nullopt;
    }
}

// ゲーム設定ファイルを保存
bool theatria::FileSystem::SaveGameSettings(const std::wstring& projectName, const theatria::GameSettingsInfo& settings)
{
    std::filesystem::path path = std::filesystem::path(projectName) / "ProjectSettings";
	std::filesystem::path fileName = "GameSettings.json";
	// プロジェクト設定フォルダが存在しない場合は作成
    if (!std::filesystem::exists(path))
    {
        if (!std::filesystem::create_directory(path))
        {
            return false; // フォルダの作成に失敗
        }
	}
	// ファイルパスを設定
	path /= fileName;

    nlohmann::ordered_json j;
    j["fileType"] = "GameSettings";
    j["startScene"] = std::filesystem::path(settings.startScene).string();
    j["frameRate"] = settings.frameRate;
    j["fixedDeltaTime"] = settings.fixedDeltaTime;
    j["debugMode"] = settings.debugMode;
	j["skyTexName"] = std::filesystem::path(settings.skyTexName).string();
	j["gravity"] = { settings.gravity.x, settings.gravity.y, settings.gravity.z };
    j["titleBar"] = std::filesystem::path(settings.titleBar).string();
    j["exeNae"] = std::filesystem::path(settings.exeName).string();

    try
    {
        std::ofstream file(path.string());
        if (!file.is_open()) return false;

        file << j.dump(4);
        file.close();
        return true;
    }
    catch (...)
    {
        return false;
    }
}

// ゲーム設定ファイルを読み込む
bool theatria::FileSystem::LoadGameSettings(const std::wstring& filePath)
{
    try
    {
        std::ifstream file(filePath);
        if (!file.is_open()) { return false; }

        nlohmann::json j;
        file >> j;

        if (j.contains("fileType") && j["fileType"] != "GameSettings")
        {
            return false;
        }

        theatria::GameSettingsInfo settings;
        settings.startScene = std::filesystem::path(j.value("startScene", "MainScene")).wstring();
        settings.frameRate = j.value("frameRate", 60);
        settings.fixedDeltaTime = j.value("fixedDeltaTime", 1.0f / 60.0f);
        settings.debugMode = j.value("debugMode", false);
		settings.skyTexName = std::filesystem::path(j.value("skyTexName", "")).wstring();
		// 重力の読み込み,値がなければデフォルト値を使用
        if (j.contains("gravity") && j["gravity"].is_array() && j["gravity"].size() >= 3)
        {
            settings.gravity = float3(
                j["gravity"][0].get<float>(),
                j["gravity"][1].get<float>(),
                j["gravity"][2].get<float>()
            );
        }
        else
        {
            settings.gravity = float3(0.0f, -9.81f, 0.0f);
        }
        settings.titleBar = std::filesystem::path(j.value("titleBar", "Theatria Engine")).wstring();
        settings.exeName = std::filesystem::path(j.value("exeName", "TheatriaGame")).wstring();
		// ゲーム設定をグローバルに保存
		theatria::FileSystem::g_GameSettings = settings;

        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool theatria::FileSystem::SaveSceneFile(const std::wstring& directory, const std::wstring& srcFileName, GameScene* scene, ECSManager* ecs, EngineCommand* engineCommand)
{
    ecs;
	// アセットフォルダが存在しない場合は作成
    std::filesystem::path dirPath = std::filesystem::path(directory) / L"Assets";
    if (!std::filesystem::exists(dirPath))
    {
        if (!std::filesystem::create_directory(dirPath))
        {
            return false; // フォルダの作成に失敗
        }
	}
    std::filesystem::path fileName = srcFileName + L".json";
    std::filesystem::path path = dirPath / fileName;
    // ファイルが存在する場合はファイル名を変更して上書き、ないなら新規作成
    if (std::filesystem::exists(path))
    {
        std::filesystem::path newFileName = scene->GetName() + L".json";
        std::filesystem::path newPath = dirPath / newFileName;
        std::filesystem::rename(path, newPath);
        path = newPath; // 新しいパスを設定
    }

    nlohmann::ordered_json j;
    j["fileType"] = "SceneFile";
    j["sceneName"] = std::filesystem::path(scene->GetName()).string();

    // オブジェクト情報を保存
    nlohmann::ordered_json objArray = nlohmann::json::array();
    for (CPrefab& prefab : scene->GetPrefabs())
    {
		nlohmann::ordered_json prefabJson;
		prefabJson["name"] = std::filesystem::path(prefab.GetName()).string();
		prefabJson["type"] = ObjectTypeToWString(prefab.GetType());
		// コンポーネントの保存
		nlohmann::ordered_json comps;
		// TransformComponentの保存
        if (IsComponentAllowedAtRuntime<TransformComponent>(prefab.GetType()))
        {
            if (const auto* t = prefab.GetComponentPtr<TransformComponent>())
            {
                comps["Transform"] = theatria::Serialization::ToJson(*t);
            }
        }
		// MeshFilterComponentの保存
        if (IsComponentAllowedAtRuntime<MeshFilterComponent>(prefab.GetType()))
        {
            if (const auto* m = prefab.GetComponentPtr<MeshFilterComponent>())
            {
                comps["MeshFilter"] = theatria::Serialization::ToJson(*m);
            }
		}
        // MeshRendererComponentの保存
        if (IsComponentAllowedAtRuntime<MeshRendererComponent>(prefab.GetType()))
        {
            if (const auto* r = prefab.GetComponentPtr<MeshRendererComponent>())
            {
                comps["MeshRenderer"] = theatria::Serialization::ToJson(*r);
            }
		}
        // MaterialComponentの保存
        if (IsComponentAllowedAtRuntime<MaterialComponent>(prefab.GetType()))
        {
            if (const auto* m = prefab.GetComponentPtr<MaterialComponent>())
            {
                comps["Material"] = theatria::Serialization::ToJson(*m);
			}
            }
        // ScriptComponentの保存
        if (IsComponentAllowedAtRuntime<ScriptComponent>(prefab.GetType()))
        {
            if (const auto* s = prefab.GetComponentPtr<ScriptComponent>())
            {
                comps["Script"] = theatria::Serialization::ToJson(*s, engineCommand->GetResourceManager()->GetScriptContainer());
            }
        }
        // LineRendererComponentの保存
        if (IsComponentAllowedAtRuntime<LineRendererComponent>(prefab.GetType()))
        {
            const auto* lineRenderers = prefab.GetAllComponentsPtr<LineRendererComponent>();
            if (lineRenderers)
            {
                comps["LineRenderer"] = theatria::Serialization::ToJson(*lineRenderers);
			}
            }
        // Rigidbody2DComponentの保存
        if (IsComponentAllowedAtRuntime<Rigidbody2DComponent>(prefab.GetType()))
        {
            if (const auto* rb = prefab.GetComponentPtr<Rigidbody2DComponent>())
            {
                comps["Rigidbody2D"] = theatria::Serialization::ToJson(*rb);
            }
        }
        // BoxCollider2DComponentの保存
        if (IsComponentAllowedAtRuntime<BoxCollider2DComponent>(prefab.GetType()))
        {
            if (const auto* bc = prefab.GetComponentPtr<BoxCollider2DComponent>())
            {
                comps["BoxCollider2D"] = theatria::Serialization::ToJson(*bc);
            }
        }
		// Rigidbody3DComponentの保存
        if(IsComponentAllowedAtRuntime<Rigidbody3DComponent>(prefab.GetType()))
        {
            if (const auto* rb3d = prefab.GetComponentPtr<Rigidbody3DComponent>())
            {
                comps["Rigidbody3D"] = theatria::Serialization::ToJson(*rb3d);
            }
		}
        // CameraComponentの保存
        if (IsComponentAllowedAtRuntime<CameraComponent>(prefab.GetType()))
        {
            if (const auto* c = prefab.GetComponentPtr<CameraComponent>())
            {
                comps["Camera"] = theatria::Serialization::ToJson(*c);
			}
            }
        // ParticleComponentの保存
        if (IsComponentAllowedAtRuntime<ParticleComponent>(prefab.GetType()))
        {
            if (const auto* p = prefab.GetComponentPtr<ParticleComponent>())
            {
                comps["Particle"] = theatria::Serialization::ToJson(*p);
            }
        }
        // EmitterComponentの保存
        if (IsComponentAllowedAtRuntime<EmitterComponent>(prefab.GetType()))
        {
            if (const auto* e = prefab.GetComponentPtr<EmitterComponent>())
            {
                comps["Emitter"] = theatria::Serialization::ToJson(*e);
			}
        }
        // UISpriteComponentの保存
        if (IsComponentAllowedAtRuntime<UISpriteComponent>(prefab.GetType()))
        {
            if (const auto* ui = prefab.GetComponentPtr<UISpriteComponent>())
            {
                comps["UISprite"] = theatria::Serialization::ToJson(*ui);
			}
        }
		// LightComponentの保存
        if (IsComponentAllowedAtRuntime<LightComponent>(prefab.GetType()))
        {
            if (const auto* l = prefab.GetComponentPtr<LightComponent>())
            {
                comps["Light"] = theatria::Serialization::ToJson(*l);
            }
        }
		// AudioComponentの保存
        if (IsComponentAllowedAtRuntime<AudioComponent>(prefab.GetType()))
        {
            if (const auto* a = prefab.GetComponentPtr<AudioComponent>())
            {
                comps["Audio"] = theatria::Serialization::ToJson(*a);
            }
		}
        // AnimationComponentの保存
        if (IsComponentAllowedAtRuntime<AnimationComponent>(prefab.GetType()))
        {
            if (const auto* anim = prefab.GetComponentPtr<AnimationComponent>())
            {
                comps["Animation"] = theatria::Serialization::ToJson(*anim);
            }
		}
        prefabJson["components"] = comps;
        objArray.push_back(prefabJson);
    }
    j["objects"] = objArray;

    // メインカメラ
    if (!scene->GetStartCameraName().empty())
    {
        j["mainCamera"] = std::filesystem::path(scene->GetStartCameraName()).string();
    }

    try
    {
        std::ofstream file(path.string());
        if (!file.is_open()) return false;
        file << j.dump(4);
        file.close();
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool theatria::FileSystem::LoadSceneFile(const std::wstring& filePath, EngineCommand* engineCommand)
{
    if (!engineCommand) { Log::Write(LogLevel::Assert, "EngineCommand is nullptr"); }
    try
    {
        std::ifstream file(filePath);
        if (!file.is_open()) return false;

        nlohmann::json j;
        file >> j;

        if (j.contains("fileType") && j["fileType"] != "SceneFile")
        {
            return false;
        }
        std::wstring sceneName;
        // シーン名設定
        if (j.contains("sceneName"))
        {
			sceneName = std::filesystem::path(j["sceneName"].get<std::string>()).wstring();
		}
		GameScene scene(sceneName);

        // オブジェクト読み込み
        if (j.contains("objects"))
        {
            for (const auto& obj : j["objects"])
            {
                std::wstring name = std::filesystem::path(obj["name"].get<std::string>()).wstring();
                ObjectType type = ObjectTypeFromString(obj["type"].get<std::string>());
                
				CPrefab prefab(name, type);

                if (!obj.contains("components")) continue;
                const auto& comps = obj["components"];

                if (comps.contains("Transform"))
                {
                    TransformComponent t{};
                    auto& jt = comps["Transform"];
					// TransformComponentの読み込み
					Deserialization::FromJson(jt,t);
                    // TransformComponentの保存
					prefab.AddComponent<TransformComponent>(t);
                }

                if (comps.contains("Camera"))
                {
                    CameraComponent c{};
                    auto& jc = comps["Camera"];
					// CameraComponentの読み込み
					Deserialization::FromJson(jc, c);
					// CameraComponentの保存
					prefab.AddComponent<CameraComponent>(c);
                }

                if (comps.contains("MeshFilter"))
                {
                    MeshFilterComponent m{};
                    if (comps["MeshFilter"].contains("modelName"))
                    {
                        std::string modelNameStr = comps["MeshFilter"]["modelName"].get<std::string>();
                        m.modelName = std::filesystem::path(modelNameStr).wstring(); // UTF-8 → wstring
                    }
                    if (comps["MeshFilter"].contains("modelID"))
                    {
                        m.modelID = comps["MeshFilter"]["modelID"];
                    }
                    // MeshFilterComponentの保存
					prefab.AddComponent<MeshFilterComponent>(m);
                }

                if (comps.contains("MeshRenderer"))
                {
                    MeshRendererComponent r{};
                    r.visible = comps["MeshRenderer"].value("visible", true);
					// MeshRendererComponentの保存
					prefab.AddComponent<MeshRendererComponent>(r);
                }

				if (comps.contains("Material"))
				{
					MaterialComponent m{};
					auto& jm = comps["Material"];
					// MaterialComponentの読み込み
					Deserialization::FromJson(jm, m);
					// MaterialComponentの保存
					prefab.AddComponent<MaterialComponent>(m);
				}

                if (comps.contains("Script"))
                {
                    ScriptComponent s{};
                    auto& js = comps["Script"];
                    // ScriptComponentの読み込み
                    Deserialization::FromJson(js, s, engineCommand->GetResourceManager()->GetScriptContainer());
                    // ScriptComponentの保存
                    prefab.AddComponent<ScriptComponent>(s);
                }

                // LineRenderer（マルチコンポーネント）
                if (comps.contains("LineRenderer"))
                {
                    for (const auto& lineJson : comps["LineRenderer"])
                    {
                        LineRendererComponent l{};
                        auto& start = lineJson["start"];
                        auto& end = lineJson["end"];
                        auto& color = lineJson["color"];

                        l.line.start = float3{ start[0], start[1], start[2] };
                        l.line.end = float3{ end[0], end[1], end[2] };
                        l.line.color = Color{ color[0], color[1], color[2], color[3] };

						// LineRendererComponentの保存
						prefab.AddComponent<LineRendererComponent>(l);
                    }
                }

                // Rigidbody2D
                if (comps.contains("Rigidbody2D"))
                {
                    Rigidbody2DComponent r{};
                    auto& jr = comps["Rigidbody2D"];
					r.isActive = jr.value("isActive", true);
                    r.isKinematic = jr.value("isKinematic", false);
                    r.gravityScale = jr.value("gravityScale", 1.0f);
                    r.mass = jr.value("mass", 1.0f);
                    r.bodyType = static_cast<physics::d2::Id2BodyType>(jr.value("bodyType", 2)); // default: b2_dynamicBody
                    r.fixedRotation = jr.value("fixedRotation", false);
					r.isCollisionStay = false;
					r.otherEntity = std::nullopt;

					// Rigidbody2DComponentの保存
					prefab.AddComponent<Rigidbody2DComponent>(r);
                }

                // BoxCollider2D
                if (comps.contains("BoxCollider2D"))
                {
                    BoxCollider2DComponent b{};
                    auto& jb = comps["BoxCollider2D"];
                    b.offsetX = jb.value("offsetX", 0.0f);
                    b.offsetY = jb.value("offsetY", 0.0f);
                    b.width = jb.value("width", 1.0f);
                    b.height = jb.value("height", 1.0f);
                    b.density = jb.value("density", 1.0f);
                    b.friction = jb.value("friction", 0.5f);
                    b.restitution = jb.value("restitution", 0.0f);
					b.isSensor = jb.value("isSensor", false);

					// BoxCollider2DComponentの保存
					prefab.AddComponent<BoxCollider2DComponent>(b);
                }

				// Rigidbody3D
                if (comps.contains("Rigidbody3D"))
                {
                    Rigidbody3DComponent r{};
					auto& jr = comps["Rigidbody3D"];
					// Rigidbody3DComponentの読み込み
					Deserialization::FromJson(jr, r);
					// Rigidbody3DComponentの保存
                    prefab.AddComponent<Rigidbody3DComponent>(r);
				}

                // Emitter
                if (comps.contains("Emitter"))
                {
                    EmitterComponent e{};
                    auto& je = comps["Emitter"];
					Deserialization::FromJson(je, e);
					// EmitterComponentの保存
					prefab.AddComponent<EmitterComponent>(e);
                }

				// Particle
                if (comps.contains("Particle"))
                {
                    ParticleComponent p{};
                    auto& jp = comps["Particle"];
                    Deserialization::FromJson(jp, p);
					// ParticleComponentの保存
					prefab.AddComponent<ParticleComponent>(p);
                }
				// UISprite
                if (comps.contains("UISprite"))
                {
                    UISpriteComponent u{};
                    auto& ju = comps["UISprite"];
                    Deserialization::FromJson(ju, u);
					// UISpriteComponentの保存
					prefab.AddComponent<UISpriteComponent>(u);
                }
                // Light
                if (comps.contains("Light"))
                {
                    LightComponent l{};
                    auto& jl = comps["Light"];
                    Deserialization::FromJson(jl, l);
                    // LightComponentの保存
					prefab.AddComponent<LightComponent>(l);
                }
				// Audio
				if (comps.contains("Audio"))
				{
					AudioComponent a{};
					auto& ja = comps["Audio"];
					Deserialization::FromJson(ja, a);
					// AudioComponentの保存
					prefab.AddComponent<AudioComponent>(a);
				}
				// Animation
                if (comps.contains("Animation"))
                {
                    AnimationComponent anim{};
                    auto& jan = comps["Animation"];
                    Deserialization::FromJson(jan, anim);
                    // AnimationComponentの保存
					prefab.AddComponent<AnimationComponent>(anim);
                }
				// Prefabの追加
				scene.AddPrefab(prefab);
            }
        }
        // メインカメラ
        if (j.contains("mainCamera"))
        {
            std::wstring camName = std::filesystem::path(j["mainCamera"].get<std::string>()).wstring();
			scene.SetStartCameraName(camName);
        }
		engineCommand->GetGameCore()->GetSceneManager()->AddScene(scene);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool theatria::FileSystem::SaveGameParameter(const std::wstring& filePath, const std::string& group, const std::string& item, const std::string& dataName, const GameParameterVariant& value)
{
    json root;

    // 既存のファイルを開く（存在しなければ新規）
    std::wstring path = m_sProjectFolderPath + L"/" + filePath + L".json";
    std::ifstream ifs(path);
    if (ifs.is_open())
    {
        try
        {
            ifs >> root;
        }
        catch (...)
        {
            return false;
        }
        ifs.close();
    }
    // ファイルタイプの確認 or 設定
    root["fileType"] = "GameParameter";
    // グループ → アイテム → データ名 に設定
    json& target = root[group][item][dataName];
    if (std::holds_alternative<int>(value))
    {
        target = std::get<int>(value);
    } else if (std::holds_alternative<float>(value))
    {
        target = std::get<float>(value);
    } else if (std::holds_alternative<bool>(value))
    {
        target = std::get<bool>(value);
    } else if (std::holds_alternative<float3>(value))
    {
        const float3& v = std::get<float3>(value);
        target = { v.x, v.y, v.z };
    }
    // 書き込み
    std::ofstream ofs(path);
    if (!ofs.is_open()) return false;
    ofs << std::setw(4) << root << std::endl;
    return true;
}

bool theatria::FileSystem::LoadGameParameter(const std::wstring& filePath, const std::string& group, const std::string& item, const std::string& dataName, GameParameterVariant& outValue)
{
    json root;
	// ファイルを開く
    std::wstring path = m_sProjectFolderPath + L"/" + filePath + L".json";
    std::ifstream ifs(path);
    if (!ifs.is_open()) { return false; }
    try
    {
        ifs >> root;
    }
    catch (...)
    {
        return false;
    }
    if (root.value("fileType", "") != "GameParameter") { return false; }
    if (!root.contains(group) || !root[group].contains(item) || !root[group][item].contains(dataName))
    {
        return false;
    }
    const json& val = root[group][item][dataName];
    if (val.is_number_integer())
    {
        outValue = val.get<int>();
    } else if (val.is_number_float())
    {
        outValue = val.get<float>();
    } else if (val.is_boolean())
    {
        outValue = val.get<bool>();
    } else if (val.is_array() && val.size() == 3 && val[0].is_number()) 
    {
        outValue = float3{ val[0].get<float>(), val[1].get<float>(), val[2].get<float>() };
    } else{
        return false;
    }
    return true;
}

bool theatria::FileSystem::SaveLaunchConfig(const LaunchConfig& config, const std::wstring& filePath)
{
    std::filesystem::path path = filePath;
    std::filesystem::path fileName = "LaunchConfig.json";
    
    // ファイルパスを設定
    path /= fileName;

    // ファイルが存在しない場合は新規作成
    if (!std::filesystem::exists(path))
    {
        std::ofstream file(path.string());
        if (!file.is_open()) return false;
        file.close();
    }

    nlohmann::ordered_json j;
    j["fileType"] = "LaunchConfig";
    j["projectName"] = config.projectName.c_str();

    try
    {
        std::ofstream file(path.string());
        if (!file.is_open()) return false;

        file << j.dump(4);
        file.close();
        return true;
    }
    catch (...)
    {
        return false;
    }
}

LaunchConfig theatria::FileSystem::LoadLaunchConfig(const std::wstring& filePath)
{
    LaunchConfig result = {};
    try
    {
        std::ifstream file(filePath);
        if (!file.is_open()) { return result; }

        nlohmann::json j;
        file >> j;

        if (j.contains("fileType") && j["fileType"] != "LaunchConfig")
        {
            return result;
        }

        result.projectName = std::filesystem::path(j.value("projectName", "")).wstring();

        return result;
    }
    catch (...)
    {
        return result;
    }
}

bool theatria::FileSystem::SaveCacheFile(const CacheFile& cache, const std::wstring& filePath)
{
    std::filesystem::path path = filePath + L"/cache/";
    std::filesystem::path fileName = "projectNameList.json";
    // プロジェクト設定フォルダが存在しない場合は作成
    if (!std::filesystem::exists(path))
    {
        if (!std::filesystem::create_directory(path))
        {
            return false; // フォルダの作成に失敗
        }
    }
    // ファイルパスを設定
    path /= fileName;

    nlohmann::ordered_json j;
    j["fileType"] = "cache";
    // wstring → string 変換（UTF-8）
    std::vector<std::string> projectNames;
    for (const auto& ws : cache.projectNames)
    {
        projectNames.emplace_back(std::filesystem::path(ws).string()); // or std::wstring_convert
    }

    j["projectNames"] = projectNames;

    try
    {
        std::ofstream file(path.string());
        if (!file.is_open()) return false;

        file << j.dump(4);
        file.close();
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool theatria::FileSystem::LoadCacheFile(const std::wstring& filePath)
{
    std::filesystem::path path = filePath + L"/cache/";
    std::filesystem::path fileName = "projectNameList.json";
    // ファイルパスを設定
    path /= fileName;
    try
    {
        std::ifstream file(path.string());
        if (!file.is_open())
        {
            return false;
        }

        nlohmann::json j;
        file >> j;

        if (j.contains("fileType") && j["fileType"] != "cache")
        {
            return false; // ファイル種別違い
        }

        theatria::CacheFile info;

        for (const auto& projectName : j["projectNames"])
        {
            std::string s = projectName.get<std::string>();
            info.projectNames.push_back(std::filesystem::path(s).wstring());
        }
        // 保存
        g_CacheFile = info;
        return true;
    }
    catch (...)
    {
        return true;
    }
}

// コンポーネントを保存
json theatria::Serialization::ToJson(const TransformComponent& t)
{
    json j;
    j["translation"] = { t.position.x, t.position.y, t.position.z };
    j["rotation"] = { t.quaternion.x, t.quaternion.y, t.quaternion.z, t.quaternion.w };
    j["scale"] = { t.scale.x, t.scale.y, t.scale.z };
	j["degrees"] = { t.degrees.x, t.degrees.y, t.degrees.z };
	j["isBillboard"] = t.isBillboard;
    return j;
}

json theatria::Serialization::ToJson(const CameraComponent& c)
{
    json j;
    j["fovAngleY"] = c.fovAngleY;
    j["aspectRatio"] = c.aspectRatio;
    j["nearZ"] = c.nearZ;
    j["farZ"] = c.farZ;
    return j;
}

json theatria::Serialization::ToJson(const MeshFilterComponent& m)
{
    json j;
    if (m.modelID.has_value())
    {
        j["modelName"] = std::filesystem::path(m.modelName).string();
        j["modelID"] = m.modelID.value();
    }
    return j;
}

json theatria::Serialization::ToJson(const MeshRendererComponent& r)
{
    json j;
    j["visible"] = r.visible;
    return j;
}

json theatria::Serialization::ToJson(const MaterialComponent& m)
{
    json j;
	j["Color"] = { m.color.r, m.color.g, m.color.b, m.color.a };
	j["enableLighting"] = m.enableLighting;
	j["enableTexture"] = m.enableTexture;
	j["shininess"] = m.shininess;
	std::string textureName = ConvertString(m.textureName);
	j["textureName"] = textureName;
    j["uvFlipY"] = m.uvFlipY;
	return j;
}

json theatria::Serialization::ToJson(const ScriptComponent& s, ScriptContainer* container)
{
	json j;
	j["scriptName"] = s.scriptName;
    ScriptData* scriptData = container->GetScriptDataByName(s.scriptName);
    if (scriptData)
    {
        for (const auto& field : scriptData->saveFields)
        {
            if (field.second.type == typeid(float))
            {
                j["fields"][field.first]["name"] = field.first;
                j["fields"][field.first]["value"] = std::get<float>(field.second.value);
                j["fields"][field.first]["type"] = "float";
                j["fields"][field.first]["minmax"] = { field.second.minmax.first, field.second.minmax.second };
            }
            else if (field.second.type == typeid(int))
            {
                j["fields"][field.first]["name"] = field.first;
                j["fields"][field.first]["value"] = std::get<int>(field.second.value);
                j["fields"][field.first]["type"] = "int";
                j["fields"][field.first]["minmax"] = { field.second.minmax.first, field.second.minmax.second };
            }
            else if (field.second.type == typeid(bool))
            {
                j["fields"][field.first]["name"] = field.first;
                j["fields"][field.first]["value"] = std::get<bool>(field.second.value);
                j["fields"][field.first]["type"] = "bool";
            }
            else if (field.second.type == typeid(float3))
            {
                float3 v = std::get<float3>(field.second.value);
                j["fields"][field.first]["name"] = field.first;
                j["fields"][field.first]["value"] = { v.x, v.y, v.z };
                j["fields"][field.first]["type"] = "float3";
            }
        }
    }
	return j;
}

json theatria::Serialization::ToJson(const std::vector<LineRendererComponent>& ls)
{
    json jArray = json::array();
    for (const auto& l : ls)
    {
        json j;
        j["start"] = { l.line.start.x, l.line.start.y, l.line.start.z };
        j["end"] = { l.line.end.x, l.line.end.y, l.line.end.z };
        j["color"] = { l.line.color.r, l.line.color.g, l.line.color.b, l.line.color.a };
        jArray.push_back(j);
    }
    return jArray;
}

json theatria::Serialization::ToJson(const Rigidbody2DComponent& rb)
{
	json j;
	j["isActive"] = rb.isActive;
	j["isKinematic"] = rb.isKinematic;
	j["gravityScale"] = rb.gravityScale;
	j["mass"] = rb.mass;
	j["bodyType"] = static_cast<int>(rb.bodyType);
	j["fixedRotation"] = rb.fixedRotation;
	return j;
}

json theatria::Serialization::ToJson(const BoxCollider2DComponent& bc)
{
	json j;
	j["offsetX"] = bc.offsetX;
	j["offsetY"] = bc.offsetY;
	j["width"] = bc.width;
	j["height"] = bc.height;
	j["density"] = bc.density;
	j["friction"] = bc.friction;   
	j["restitution"] = bc.restitution;
	j["isSensor"] = bc.isSensor;
	return j;
}

json theatria::Serialization::ToJson(const Rigidbody3DComponent& rb)
{
    json j;
	j["active"] = rb.IsActive();
	j["halfsize"] = { rb.halfsize.x, rb.halfsize.y, rb.halfsize.z };
	j["friction"] = rb.friction;
	j["restitution"] = rb.restitution;
	j["fixedPositionX"] = rb.fixedPositionX;
	j["fixedPositionY"] = rb.fixedPositionY;
    j["fixedPositionZ"] = rb.fixedPositionZ;
	j["fixedRotationX"] = rb.fixedRotationX;
	j["fixedRotationY"] = rb.fixedRotationY;
	j["fixedRotationZ"] = rb.fixedRotationZ;
	j["velocity"] = { rb.velocity.x, rb.velocity.y, rb.velocity.z };
	j["gravityScale"] = rb.gravityScale;
	j["bodyType"] = static_cast<int>(rb.bodyType);
	j["mass"] = rb.mass;
	j["isSensor"] = rb.isSensor;
	j["quaternion"] = { rb.quaternion.x, rb.quaternion.y, rb.quaternion.z, rb.quaternion.w };
	return j;
}

json theatria::Serialization::ToJson(const EmitterComponent& e)
{
    json j;
    j["lifeTime"] = { e.lifeTime.median,e.lifeTime.amplitude };
    {
        // position
        {// value
            json x; json y; json z;
            x["x"] = { e.position.value.x.median, e.position.value.x.amplitude};
			y["y"] = { e.position.value.y.median, e.position.value.y.amplitude };
            z["z"] = { e.position.value.z.median, e.position.value.z.amplitude };
			j["position"]["value"] = { x, y, z };
        }
        {// velocity
            json x; json y; json z;
			x["x"] = { e.position.velocity.x.median, e.position.velocity.x.amplitude };
			y["y"] = { e.position.velocity.y.median, e.position.velocity.y.amplitude };
			z["z"] = { e.position.velocity.z.median, e.position.velocity.z.amplitude };
            j["position"]["velocity"] = { x, y, z };
        }
        {// acceleration
			json x; json y; json z;
			x["x"] = { e.position.acceleration.x.median, e.position.acceleration.x.amplitude };
			y["y"] = { e.position.acceleration.y.median, e.position.acceleration.y.amplitude };
			z["z"] = { e.position.acceleration.z.median, e.position.acceleration.z.amplitude };
            j["position"]["acceleration"] = { x, y, z };
        }
        // rotation
        {// value
            json x; json y; json z;
            x["x"] = { e.rotation.value.x.median, e.rotation.value.x.amplitude };
            y["y"] = { e.rotation.value.y.median, e.rotation.value.y.amplitude };
            z["z"] = { e.rotation.value.z.median, e.rotation.value.z.amplitude };
            j["rotation"]["value"] = { x, y, z };
        }
        {// velocity
            json x; json y; json z;
            x["x"] = { e.rotation.velocity.x.median, e.rotation.velocity.x.amplitude };
            y["y"] = { e.rotation.velocity.y.median, e.rotation.velocity.y.amplitude };
            z["z"] = { e.rotation.velocity.z.median, e.rotation.velocity.z.amplitude };
            j["rotation"]["velocity"] = { x, y, z };
        }
        {// acceleration
            json x; json y; json z;
            x["x"] = { e.rotation.acceleration.x.median, e.rotation.acceleration.x.amplitude };
            y["y"] = { e.rotation.acceleration.y.median, e.rotation.acceleration.y.amplitude };
            z["z"] = { e.rotation.acceleration.z.median, e.rotation.acceleration.z.amplitude };
            j["rotation"]["acceleration"] = { x, y, z };
        }
        // scale
        {// value
            json x; json y; json z;
            x["x"] = { e.scale.value.x.median, e.scale.value.x.amplitude };
            y["y"] = { e.scale.value.y.median, e.scale.value.y.amplitude };
            z["z"] = { e.scale.value.z.median, e.scale.value.z.amplitude };
            j["scale"]["value"] = { x, y, z };
        }
        {// velocity
            json x; json y; json z;
            x["x"] = { e.scale.velocity.x.median, e.scale.velocity.x.amplitude };
            y["y"] = { e.scale.velocity.y.median, e.scale.velocity.y.amplitude };
            z["z"] = { e.scale.velocity.z.median, e.scale.velocity.z.amplitude };
            j["scale"]["velocity"] = { x, y, z };
        }
        {// acceleration
            json x; json y; json z;
            x["x"] = { e.scale.acceleration.x.median, e.scale.acceleration.x.amplitude };
            y["y"] = { e.scale.acceleration.y.median, e.scale.acceleration.y.amplitude };
            z["z"] = { e.scale.acceleration.z.median, e.scale.acceleration.z.amplitude };
            j["scale"]["acceleration"] = { x, y, z };
		}
    }
	j["frequency"] = e.frequency;
	j["frequencyTime"] = e.frequencyTime;
	j["emitCount"] = e.emitCount;
	j["isFadeOut"] = e.isFadeOut;
	j["isBillboard"] = e.isBillboard;
	return j;
}

json theatria::Serialization::ToJson(const ParticleComponent& p)
{
    json j;
	j["count"] = p.count;
	return j;
}

json theatria::Serialization::ToJson(const UISpriteComponent& ui)
{
    json j;
	j["position"] = { ui.position.x, ui.position.y };
    j["rotation"] = ui.rotation;
	j["scale"] = { ui.scale.x, ui.scale.y };
	j["anchorPoint"] = { ui.anchorPoint.x, ui.anchorPoint.y }; 
	j["size"] = { ui.size.x, ui.size.y };
	j["textureLeftTop"] = { ui.textureLeftTop.x, ui.textureLeftTop.y };
	j["textureSize"] = { ui.textureSize.x, ui.textureSize.y };
	return j;
}

json theatria::Serialization::ToJson(const LightComponent& l)
{
	json j;
	j["color"] = { l.color.r, l.color.g, l.color.b, l.color.a };
	j["intensity"] = l.intensity;
	j["range"] = l.range;
	j["decay"] = l.decay;
	j["spotAngle"] = l.spotAngle;
	j["spotFalloffStart"] = l.spotFalloffStart;
	j["type"] = static_cast<int>(l.type);
	j["active"] = l.active;
	return j;
}

json theatria::Serialization::ToJson(const AudioComponent& a)
{
    json j;
	//j["audioName"] = a.audioName;
	j["isLoop"] = a.isLoop;
	return j;
}

json theatria::Serialization::ToJson(const AnimationComponent& a)
{
	json j;
    j["transitionDuration"] = a.transitionDuration;
	j["animationIndex"] = a.animationIndex;
	std::string modelNameStr = ConvertString(a.modelName);
    j["modelName"] = modelNameStr;

	return j;
}

FileType theatria::FileSystem::GetJsonFileType(const std::filesystem::path& path)
{
    try
    {
        std::ifstream file(path);
        if (!file.is_open()) return FileType::Unknown;

        nlohmann::json j;
        file >> j;

        std::string type = j.value("fileType", "");
        if (type == "ChoProject")   return FileType::ChoProject;
        if (type == "EngineConfig") return FileType::EngineConfig;
        if (type == "GameSettings") return FileType::GameSettings;
        if (type == "SceneFile")    return FileType::SceneFile;
        if (type == "ModelFile")    return FileType::ModelFile;
        if (type == "ImageFile")    return FileType::ImageFile;
        if (type == "SoundFile")    return FileType::SoundFile;
        if (type == "EffectFile")   return FileType::EffectFile;
        if (type == "ScriptFile")   return FileType::ScriptFile;

        return FileType::Unknown;
    }
    catch (...)
    {
        return FileType::Unknown;
    }
}

void theatria::FileSystem::SaveProject(EditorManager* editorManager, SceneManager* sceneManager, GameWorld* gameWorld, ECSManager* ecs, EngineCommand* engineCommand)
{
    gameWorld; sceneManager;
    if (m_sProjectName.empty()) { return; }
    // セーブ
	// GameSettingsFile
    // Skyboxtexの保存
    std::wstring skyTexName = editorManager->GetEngineCommand()->GetResourceManager()->GetSkyboxTextureName();
    g_GameSettings.skyTexName = skyTexName;
	// 重力の保存
	g_GameSettings.gravity = editorManager->GetEngineCommand()->GetGameCore()->GetPhysicsWorld3D()->GetGravity();
    theatria::FileSystem::SaveGameSettings(m_sProjectFolderPath, g_GameSettings);
    // SceneFile
	// 編集されたシーンを保存
    editorManager->SaveEditingScene();
    for (auto& scene : editorManager->GetSceneMap())
    {
		// シーンファイルを保存
        theatria::FileSystem::SaveSceneFile(
            m_sProjectFolderPath,
            scene.first,
            editorManager->GetEditScene(scene.first),
            ecs,
            engineCommand
		);
    }
    // キャッシュファイルの保存
    SaveCacheFile(g_CacheFile, std::filesystem::current_path().wstring());
}

// プロジェクトフォルダを読み込む
bool theatria::FileSystem::LoadProjectFolder(const std::wstring& projectFolderPath, EngineCommand* engineCommand)
{
    std::filesystem::path path = projectFolderPath;
    m_sProjectName = path.filename().wstring();
    m_sProjectFolderPath = projectFolderPath;

    // プロジェクトファイル類を読み込み
    // 全ファイル走査（サブディレクトリ含む）
    ScanFolder(path,engineCommand);
    engineCommand->GetResourceManager()->SetSkyboxTextureName(g_GameSettings.skyTexName);
	float3 gravityfloat3 = g_GameSettings.gravity;
    std::unique_ptr<SetGravityCommand> setGravity = std::make_unique<SetGravityCommand>(gravityfloat3);
    engineCommand->ExecuteCommand(std::move(setGravity));
	// 最初のシーンをロード
    engineCommand->GetEditorManager()->ChangeEditingScene(g_GameSettings.startScene);
    // ステージングされたdllをロード
    ScriptProject::LoadScriptDLL();
    return true;
}

/// CSVを2次元vectorに読み込む関数
std::vector<std::vector<std::string>> theatria::FileSystem::LoadCSV(const std::string& filePath)
{
    std::vector<std::vector<std::string>> data;
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        std::cerr << "CSVファイルを開けませんでした: " << filePath << std::endl;
        return data;
    }

    std::string line;
    while (std::getline(file, line))
    {
        std::vector<std::string> row;
        std::stringstream ss(line);
        std::string cell;

        while (std::getline(ss, cell, ',')) // カンマ区切りで取得
        {
            row.push_back(cell);
        }

        data.push_back(row);
    }

    return data;
}

/// CSVを2次元vector<int>に読み込む関数
std::vector<std::vector<int>> theatria::FileSystem::LoadCSV_Int(const std::string& filePath)
{
    std::vector<std::vector<int>> data;
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        std::cerr << "CSVファイルを開けませんでした: " << filePath << std::endl;
        return data;
    }

    std::string line;
    while (std::getline(file, line))
    {
        std::vector<int> row;
        std::stringstream ss(line);
        std::string cell;

        while (std::getline(ss, cell, ',')) // カンマ区切りで取得
        {
            if (!cell.empty())
            {
                try
                {
                    row.push_back(std::stoi(cell)); // string → int
                }
                catch (const std::invalid_argument&)
                {
                    std::cerr << "数値変換エラー: " << cell << std::endl;
                    row.push_back(0); // エラー時は0にしておく
                }
            }
            else
            {
                row.push_back(0); // 空セルは0扱い
            }
        }

        data.push_back(row);
    }

    return data;
}

std::string theatria::FileSystem::GenerateGUID()
{
    GUID guid;
    HRESULT hr;
    hr = CoCreateGuid(&guid);
    if (FAILED(hr))
    {
        std::cerr << "Failed to generate GUID. Error code: " << hr << "\n";
        return "";
    }

    std::ostringstream oss;
    oss << std::hex << std::uppercase;
    oss << "{"
        << std::setw(8) << std::setfill('0') << guid.Data1 << "-"
        << std::setw(4) << std::setfill('0') << guid.Data2 << "-"
        << std::setw(4) << std::setfill('0') << guid.Data3 << "-";

    for (int i = 0; i < 2; ++i)
    {
        oss << std::setw(2) << std::setfill('0') << static_cast<int>(guid.Data4[i]);
    }
    oss << "-";
    for (int i = 2; i < 8; ++i)
    {
        oss << std::setw(2) << std::setfill('0') << static_cast<int>(guid.Data4[i]);
    }
    oss << "}";

    return oss.str();
}

void theatria::FileSystem::ScriptProject::GenerateSolutionAndProject()
{
    std::string projectNameStr = ConvertString(m_sProjectName);
    // 出力先
    std::string outputPath = ConvertString(m_sProjectFolderPath);
	// ソリューションファイルパス
    m_SlnPath = outputPath + "/" + projectNameStr + ".sln";
	// vcxprojファイルパス
	m_ProjPath = outputPath + "/" + projectNameStr + ".vcxproj";
	// フィルターファイルパス
	std::string filterName = outputPath + "/" + projectNameStr + ".vcxproj.filters";
    // プロジェクトファイルの生成
    UpdateVcxproj();
    // フィルターファイルの生成
    UpdateFilters(filterName);
	// ソリューションファイルの生成
    if (!fs::exists(m_SlnPath))
    {
        std::ofstream slnFile(m_SlnPath, std::ios::trunc);
        slnFile << "Microsoft Visual Studio Solution File, Format Version 12.00\n";
        slnFile << "# Visual Studio Version 17\n";
        slnFile << "VisualStudioVersion = 17.0.31903.59\n";
        slnFile << "MinimumVisualStudioVersion = 10.0.40219.1\n";

        slnFile << "Project(\"" << m_SlnGUID << "\") = \"" << projectNameStr << "\", \"" << projectNameStr << ".vcxproj\", \"" << m_ProjGUID << "\"\n";
        slnFile << "EndProject\n";

        // ソリューション構成の追加
        slnFile << "Global\n";
        slnFile << "    GlobalSection(SolutionConfigurationPlatforms) = preSolution\n";
        slnFile << "        Debug|x64 = Debug|x64\n";
        slnFile << "        Release|x64 = Release|x64\n";
        slnFile << "    EndGlobalSection\n";
        slnFile << "    GlobalSection(ProjectConfigurationPlatforms) = postSolution\n";
        slnFile << "        " << m_ProjGUID << ".Debug|x64.ActiveCfg = Debug|x64\n";
        slnFile << "        " << m_ProjGUID << ".Debug|x64.Build.0 = Debug|x64\n";
        slnFile << "        " << m_ProjGUID << ".Release|x64.ActiveCfg = Release|x64\n";
        slnFile << "        " << m_ProjGUID << ".Release|x64.Build.0 = Release|x64\n";
        slnFile << "    EndGlobalSection\n";
        slnFile << "    GlobalSection(SolutionProperties) = preSolution\n";
        slnFile << "        HideSolutionNode = FALSE\n";
        slnFile << "    EndGlobalSection\n";
        slnFile << "EndGlobal\n";

        slnFile.close();
        std::cout << "Generated solution file: " << m_SlnPath << "\n";
    } else
    {
        std::cout << "Solution file already exists: " << m_SlnPath << "\n";
    }
}

void theatria::FileSystem::ScriptProject::UpdateVcxproj()
{
    std::vector<std::string> scriptFiles;

    fs::path exePath = fs::current_path(); // 実行ファイルがある場所
    // プロジェクトディレクトリ全体を再帰的に探索
    fs::path projectDir = exePath / m_sProjectFolderPath;

    for (const auto& entry : fs::recursive_directory_iterator(projectDir))
    {
        if (!entry.is_regular_file()) continue;

        const auto& path = entry.path();
        std::string ext = path.extension().string();

        if (ext == ".cpp" || ext == ".h")
        {
            fs::path relativePath = fs::relative(path, projectDir); // 相対パス化
            relativePath.make_preferred(); // Windowsの \ 区切りに変換
            scriptFiles.push_back(relativePath.string()); // 相対パスを保存
        }
    }

    // パス設定
    //fs::path currentPath = fs::current_path();

    // インクルードディレクトリ
	// スクリプトファイルのパス
    fs::path includeBase = std::filesystem::current_path();
    includeBase = includeBase.parent_path();
    includeBase = includeBase /= "$(Configuration)";
    fs::path systemPath = includeBase / "Cho";
    fs::path mathLibPath = includeBase / "ChoMath/include";
    fs::path mathPath = includeBase / "ChoMath";
    fs::path physicsPath = includeBase / "PhysicsEngine/include";
    fs::path scriptPath = includeBase / "Cho/GameCore/Marionnette";
	fs::path contextPath = includeBase / "Cho/GameCore/ScriptAPI";
    fs::path projectDirPath = "$(ProjectDir)";
    fs::path projectDirPath2 = "$(ProjectDir)Assets/Scripts";

    // ライブラリディレクトリ
    fs::path libraryPath = std::filesystem::current_path();
    fs::path libraryPath2 = "$(ProjectDir)";

    // パスの正規化
    systemPath.make_preferred();
    mathLibPath.make_preferred();
    physicsPath.make_preferred();
	scriptPath.make_preferred();
	contextPath.make_preferred();

    libraryPath.make_preferred();
	libraryPath2.make_preferred();

    std::ofstream vcxFile(m_ProjPath, std::ios::trunc);
    vcxFile << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    vcxFile << "<Project DefaultTargets=\"Build\" ToolsVersion=\"17.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n";

    // プロジェクト構成の定義
    vcxFile << "  <ItemGroup Label=\"ProjectConfigurations\">\n";
    vcxFile << "    <ProjectConfiguration Include=\"Debug|x64\">\n";
    vcxFile << "      <Configuration>Debug</Configuration>\n";
    vcxFile << "      <Platform>x64</Platform>\n";
    vcxFile << "    </ProjectConfiguration>\n";
    vcxFile << "    <ProjectConfiguration Include=\"Release|x64\">\n";
    vcxFile << "      <Configuration>Release</Configuration>\n";
    vcxFile << "      <Platform>x64</Platform>\n";
    vcxFile << "    </ProjectConfiguration>\n";
    vcxFile << "  </ItemGroup>\n";

    // グローバルプロパティ
    vcxFile << "  <PropertyGroup Label=\"Globals\">\n";
    vcxFile << "    <ProjectGuid>" << m_ProjGUID << "</ProjectGuid>\n";
    vcxFile << "    <Keyword>Win32Proj</Keyword>\n";
    vcxFile << "    <RootNamespace>" << ConvertString(m_sProjectName) << "</RootNamespace>\n";
    vcxFile << "  </PropertyGroup>\n";

    vcxFile << "  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.Default.props\" />\n";

    // Debug 構成
    vcxFile << "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Debug|x64'\" Label=\"Configuration\">\n";
    vcxFile << "    <ConfigurationType>DynamicLibrary</ConfigurationType>\n";
    vcxFile << "    <UseDebugLibraries>true</UseDebugLibraries>\n";
    vcxFile << "    <PlatformToolset>v143</PlatformToolset>\n";
    vcxFile << "    <CharacterSet>Unicode</CharacterSet>\n";
    vcxFile << "  </PropertyGroup>\n";

    // Release 構成
    vcxFile << "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Release|x64'\" Label=\"Configuration\">\n";
    vcxFile << "    <ConfigurationType>DynamicLibrary</ConfigurationType>\n";
    vcxFile << "    <UseDebugLibraries>false</UseDebugLibraries>\n";
    vcxFile << "    <PlatformToolset>v143</PlatformToolset>\n";
    vcxFile << "    <WholeProgramOptimization>true</WholeProgramOptimization>\n";
    vcxFile << "    <CharacterSet>Unicode</CharacterSet>\n";
    vcxFile << "  </PropertyGroup>\n";

    vcxFile << "  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.props\" />\n";

    // 共通プロパティ
    vcxFile << "  <PropertyGroup>\n";
    vcxFile << "    <OutDir>$(SolutionDir)bin\\</OutDir>\n";
    vcxFile << "    <IntDir>$(SolutionDir)obj\\</IntDir>\n";
    vcxFile << "    <TargetName>" << ConvertString(m_sProjectName) << "</TargetName>\n";
    vcxFile << "  </PropertyGroup>\n";

    // Debug 用設定
    vcxFile << "  <ItemDefinitionGroup Condition=\"'$(Configuration)|$(Platform)'=='Debug|x64'\">\n";
    vcxFile << "    <ClCompile>\n";
    vcxFile << "      <WarningLevel>Level3</WarningLevel>\n";
    vcxFile << "      <Optimization>Disabled</Optimization>\n";
    vcxFile << "      <MultiProcessorCompilation>true</MultiProcessorCompilation>\n";
    vcxFile << "      <PreprocessorDefinitions>_DEBUG;EXPORT_SCRIPT_API;%(PreprocessorDefinitions)</PreprocessorDefinitions>\n";
    vcxFile << "      <AdditionalIncludeDirectories>" << projectDirPath.string() << ";" << projectDirPath2.string() << ";" << physicsPath.string() << ";" << contextPath.string() << ";" << scriptPath.string() << ";" << mathLibPath.string() << ";" << mathPath.string() << ";" << systemPath.string() << ";" << ";%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>\n";
    vcxFile << "      <LanguageStandard>stdcpp20</LanguageStandard>\n";
    vcxFile << "      <LanguageStandard_C>stdc17</LanguageStandard_C>\n";
    vcxFile << "      <AdditionalOptions>/utf-8 %(AdditionalOptions)</AdditionalOptions>\n";
    vcxFile << "      <RuntimeLibrary>MultiThreadedDebugDLL</RuntimeLibrary>\n"; // MDd
    vcxFile << "    </ClCompile>\n";
    vcxFile << "    <Link>\n";
    vcxFile << "      <SubSystem>Windows</SubSystem>\n";
    vcxFile << "      <GenerateDebugInformation>true</GenerateDebugInformation>\n";
    vcxFile << "      <AdditionalDependencies>"<<"ChoMath.lib"<<";"<< "TheatriaEngine.lib" << ";" << "%(AdditionalDependencies)</AdditionalDependencies>\n";
    vcxFile << "      <AdditionalLibraryDirectories>" << libraryPath.string() << ";" << libraryPath2.string() << ";%(AdditionalLibraryDirectories)</AdditionalLibraryDirectories>\n";
    vcxFile << "    </Link>\n";
    vcxFile << "  </ItemDefinitionGroup>\n";

    // Release 用設定
    vcxFile << "  <ItemDefinitionGroup Condition=\"'$(Configuration)|$(Platform)'=='Release|x64'\">\n";
    vcxFile << "    <ClCompile>\n";
    vcxFile << "      <WarningLevel>Level3</WarningLevel>\n";
    //vcxFile << "      <Optimization>MaxSpeed</Optimization>\n";
    vcxFile << "      <Optimization>Disabled</Optimization>\n";
    vcxFile << "      <MultiProcessorCompilation>true</MultiProcessorCompilation>\n";
    vcxFile << "      <PreprocessorDefinitions>NDEBUG;EXPORT_SCRIPT_API;%(PreprocessorDefinitions)</PreprocessorDefinitions>\n";
    vcxFile << "      <AdditionalIncludeDirectories>" << projectDirPath.string() << ";" << projectDirPath2.string() << ";" << physicsPath.string() << ";" << contextPath.string() << ";" << scriptPath.string() << ";" << mathLibPath.string() << ";" << mathPath.string() << ";" << systemPath.string() << ";" << ";%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>\n";
    vcxFile << "      <LanguageStandard>stdcpp20</LanguageStandard>\n";
    vcxFile << "      <LanguageStandard_C>stdc17</LanguageStandard_C>\n";
    vcxFile << "      <AdditionalOptions>/utf-8 %(AdditionalOptions)</AdditionalOptions>\n";
    vcxFile << "      <RuntimeLibrary>MultiThreadedDLL</RuntimeLibrary>\n"; // MD
    vcxFile << "    </ClCompile>\n";
    vcxFile << "    <Link>\n";
    vcxFile << "      <SubSystem>Windows</SubSystem>\n";
    vcxFile << "      <EnableCOMDATFolding>true</EnableCOMDATFolding>\n";
    vcxFile << "      <OptimizeReferences>true</OptimizeReferences>\n";
    vcxFile << "      <GenerateDebugInformation>true</GenerateDebugInformation>\n";
    vcxFile << "      <AdditionalDependencies>" << "ChoMath.lib" << ";" << "TheatriaEngine.lib" << ";" << "%(AdditionalDependencies)</AdditionalDependencies>\n";
    vcxFile << "      <AdditionalLibraryDirectories>" << libraryPath.string() << ";" << libraryPath2.string() << ";%(AdditionalLibraryDirectories)</AdditionalLibraryDirectories>\n";
    vcxFile << "    </Link>\n";
    vcxFile << "  </ItemDefinitionGroup>\n";

    // ソースファイルの登録
    vcxFile << "  <ItemGroup>\n";
    for (const auto& file : scriptFiles)
    {
        if (file.ends_with(".cpp"))
        {
            vcxFile << "    <ClCompile Include=\"" << file << "\" />\n";
        }
    }
    vcxFile << "  </ItemGroup>\n";

    // ヘッダーファイルの登録
    vcxFile << "  <ItemGroup>\n";
    for (const auto& file : scriptFiles)
    {
        if (file.ends_with(".h"))
        {
            vcxFile << "    <ClInclude Include=\"" << file << "\" />\n";
        }
    }
    vcxFile << "  </ItemGroup>\n";

    vcxFile << "  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\" />\n";
    vcxFile << "</Project>\n";
    vcxFile.close();
}

void theatria::FileSystem::ScriptProject::UpdateFilters(const std::string& filterPath)
{
    std::vector<std::string> scriptFiles;

    fs::path exePath = fs::current_path(); // 実行ファイルがある場所
    // プロジェクトディレクトリ全体を再帰的に探索
    fs::path projectDir = exePath / m_sProjectFolderPath;

    // scriptFiles には相対パスを格納するように修正
    for (const auto& entry : fs::recursive_directory_iterator(projectDir))
    {
        if (!entry.is_regular_file()) continue;

        const auto& path = entry.path();
        std::string ext = path.extension().string();

        if (ext == ".cpp" || ext == ".h")
        {
            // .vcxproj があるディレクトリからの相対パスを取得
            std::string relativePath = fs::relative(path, projectDir).make_preferred().string();
            scriptFiles.push_back(relativePath);
        }
    }

    std::ofstream filtersFile(filterPath, std::ios::trunc);
    filtersFile << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    filtersFile << "<Project ToolsVersion=\"4.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n";
    filtersFile << "  <ItemGroup>\n";

    for (const auto& file : scriptFiles)
    {
        if (file.ends_with(".cpp"))
        {
            filtersFile << "    <ClCompile Include=\"" << file << "\">\n";
            filtersFile << "      <Filter>Source Files</Filter>\n";
            filtersFile << "    </ClCompile>\n";
        } else if (file.ends_with(".h"))
        {
            filtersFile << "    <ClInclude Include=\"" << file << "\">\n";
            filtersFile << "      <Filter>Header Files</Filter>\n";
            filtersFile << "    </ClInclude>\n";
        }
    }

    filtersFile << "  </ItemGroup>\n";
    filtersFile << "  <ItemGroup>\n";
    filtersFile << "    <Filter Include=\"Source Files\">\n";
    filtersFile << "      <UniqueIdentifier>{E2E5D8EC-87BD-45C3-8B23-4E6F9D5E8DB9}</UniqueIdentifier>\n";
    filtersFile << "    </Filter>\n";
    filtersFile << "    <Filter Include=\"Header Files\">\n";
    filtersFile << "      <UniqueIdentifier>{B6B78A4F-BEAF-40F2-927B-73A40EC5C5B4}</UniqueIdentifier>\n";
    filtersFile << "    </Filter>\n";
    filtersFile << "  </ItemGroup>\n";
    filtersFile << "</Project>\n";

    filtersFile.close();
}

void theatria::FileSystem::ScriptProject::GenerateScriptFiles(const std::string& scriptName)
{
    std::filesystem::path outputDir = m_sProjectFolderPath;

    // テンプレートファイルのパス
    std::filesystem::path templateHeader = "Cho/Resources/EngineAssets/TemplateScript/TemplateScript.h";
    std::filesystem::path templateCpp = "Cho/Resources/EngineAssets/TemplateScript/TemplateScript.cpp";

    // 出力ファイル名
    std::string headerFileName = scriptName + ".h";
    std::string cppFileName = scriptName + ".cpp";

    // ヘルパー関数：テンプレートの置換
    auto ReplaceScriptName = [&](const std::string& content) -> std::string {
        std::string result = content;
        std::regex pattern("\\{\\s*SCRIPT_NAME\\s*\\}");
        return std::regex_replace(result, pattern, scriptName);
        };

    // ヘッダーファイル生成
    {
        std::ifstream in(templateHeader);
        if (!in)
        {
            throw std::runtime_error("TemplateScript.h not found");
        }
        std::stringstream buffer;
        buffer << in.rdbuf();
        std::string replaced = ReplaceScriptName(buffer.str());

        std::ofstream out(outputDir / headerFileName);
        out << replaced;
    }

    // CPPファイル生成
    {
        std::ifstream in(templateCpp);
        if (!in)
        {
            throw std::runtime_error("TemplateScript.cpp not found");
        }
        std::stringstream buffer;
        buffer << in.rdbuf();
        std::string replaced = ReplaceScriptName(buffer.str());

        std::ofstream out(outputDir / cppFileName);
        out << replaced;
    }
}

void theatria::FileSystem::ScriptProject::LoadProjectPath(const std::wstring& projectName)
{
    std::string projectNameStr = ConvertString(projectName);
    // 出力先
    std::string outputPath = ConvertString(m_sProjectFolderPath);
    // ソリューションファイルパス
    m_SlnPath = outputPath + "/" + projectNameStr + ".sln";
    // vcxprojファイルパス
    m_ProjPath = outputPath + "/" + projectNameStr + ".vcxproj";
}

void theatria::FileSystem::ScriptProject::LoadScriptDLL()
{
    // アンロード
    UnloadScriptDLL();
	// DLLのパス
	// std::string dllPath = ConvertString(m_sProjectFolderPath) + "/bin/" + ConvertString(m_sProjectName) + ".dll";
    fs::path dllPath = fs::path(m_sProjectFolderPath) / L"bin" / (m_sProjectName + L".dll");
    // DLLをステージング
    auto staged = StageDllAndPdbInSiblingFolder(dllPath);
    if (staged.stagedDll.empty())
    {
        Log::Write(LogLevel::Info, L"DLL not found: " + dllPath.wstring());
        return;
    }
    if(staged.stagedPdb.empty())
    {
        Log::Write(LogLevel::Info, L"PDB not found for DLL: " + dllPath.wstring());
    }

	// PDBのロード
	LoadPDB(staged.stagedDll.string());
    // ロード
	m_DllHandle = LoadLibraryA(staged.stagedDll.string().c_str());
    if (!m_DllHandle)
    {
		Log::Write(LogLevel::Info, L"Failed to load DLL: " + dllPath.wstring());
        return;
    }
}

void theatria::FileSystem::ScriptProject::UnloadScriptDLL()
{
	// PDBのアンロード
    UnloadPDB();
	// DLLのアンロード
	FreeLibrary(m_DllHandle);
    m_DllHandle = nullptr;
}

//bool Cho::FileSystem::ScriptProject::BuildScriptDLL()
//{
//	std::string projectPath = "GameProjects/" + ConvertString(m_sProjectName) + "/" + ConvertString(m_sProjectName) + ".vcxproj";
//    std::string command = "msbuild \"" + projectPath + "\" /p:Configuration=Debug /p:Platform=x64";
//    int result = std::system(command.c_str());
//
//    if (result != 0)
//    {
//		Log::Write(LogLevel::Error, "Script project build failed with code " + std::to_string(result));
//		return false;
//    } else
//    {
//		Log::Write(LogLevel::Info, "Script project built successfully.");
//		return true;
//    }
//}

bool theatria::FileSystem::ScriptProject::BuildScriptDLL()
{
    std::string projectPath = ConvertString(m_sProjectFolderPath) + "/" + ConvertString(m_sProjectName) + ".vcxproj";
    std::string command = "msbuild \"" + projectPath + "\" /p:Configuration=Debug /p:Platform=x64";

    std::ostringstream logStream;
    logStream << "実行したコマンド: " << command << "\n";

    // _popen を利用してコマンド出力を取得
    FILE* pipe = _popen(command.c_str(), "r");
    if (!pipe)
    {
        Log::Write(LogLevel::Error, "コマンド実行に失敗しました: " + command);
        return false;
    }

    char buffer[128] = { 0 };
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
    {
        logStream << buffer;
    }

    int result = _pclose(pipe);
    Log::Write(LogLevel::Info, logStream.str());

    if (result != 0)
    {
        Log::Write(LogLevel::Error, "Script project build failed with code " + std::to_string(result));
        return false;
    } else
    {
        Log::Write(LogLevel::Info, "Script project built successfully.");
        return true;
    }
}

std::vector<std::string> theatria::FileSystem::ScriptProject::GetScriptFiles()
{
    std::vector<std::string> scriptNames;
    fs::path exePath = fs::current_path(); // 実行ファイルがある場所
    // プロジェクトディレクトリ全体を再帰的に探索
    fs::path projectDir = exePath / m_sProjectFolderPath;
    for (const auto& entry : fs::recursive_directory_iterator(projectDir))
    {
        if (!entry.is_regular_file()) continue;

        const fs::path& path = entry.path();
        std::string ext = path.extension().string();

        // .cpp または .h のみに絞る
        if (ext == ".cpp" || ext == ".h")
        {
            // ファイル名（拡張子なし）
            std::string name = path.stem().string(); // 例: PlayerController.cpp → "PlayerController"

            // 重複チェック（同名 .cpp / .h がある場合）
            if (std::find(scriptNames.begin(), scriptNames.end(), name) == scriptNames.end())
            {
                scriptNames.push_back(name);
            }
        }
    }
	return scriptNames;
}

bool theatria::FileSystem::ScriptProject::LoadPDB(const path& dllPath)
{
    HANDLE process = GetCurrentProcess();

    // 初期化
    static bool initialized = false;
    if (!initialized)
    {
        if (!SymInitialize(process, nullptr, TRUE))
        {
            DWORD error = GetLastError();
            std::cerr << "SymInitialize failed (Error Code: " << error << ")\n";
            return false;
        }
        initialized = true;
    }

    // DLLを読み込んで、PDBも自動で探してロード
    DWORD64 baseAddress = SymLoadModuleExW(
        process,
        nullptr,
        dllPath.c_str(),
        nullptr,
        0,  // 0 = 自動計算
        0,
        nullptr,
        0
    );

    if (baseAddress == 0)
    {
        DWORD error = GetLastError();
        std::cerr << "SymLoadModuleEx failed (Error Code: " << error << ")\n";
        return false;
    }

    std::cout << "DLL and PDB loaded successfully.\n";

    // 後でUnloadに使う
    m_PDBBaseAddress = baseAddress;

    return true;
}

void theatria::FileSystem::ScriptProject::UnloadPDB()
{
    SymUnloadModule64(GetCurrentProcess(), m_PDBBaseAddress);
	SymCleanup(GetCurrentProcess());
}

bool theatria::FileSystem::ScriptProject::WaitForBuild(const std::wstring& dllPath, int timeoutMs)
{
    using namespace std::chrono;

    // DLLをアンロード
    UnloadScriptDLL();

    auto start = steady_clock::now();

    FILETIME lastWrite = {};
    bool updated = false;

    while (duration_cast<milliseconds>(steady_clock::now() - start).count() < timeoutMs)
    {
        WIN32_FILE_ATTRIBUTE_DATA fad;
        if (GetFileAttributesExW(dllPath.c_str(), GetFileExInfoStandard, &fad))
        {
            if (CompareFileTime(&lastWrite, &fad.ftLastWriteTime) != 0)
            {
                // 更新された
                lastWrite = fad.ftLastWriteTime;
                updated = true;
            }

            if (updated)
            {
                // 書き込み完了チェック: 読み取り専用で開けるかどうか
                HANDLE hFile = CreateFileW(
                    dllPath.c_str(),
                    GENERIC_READ,
                    FILE_SHARE_READ,  // 読み込みだけ共有
                    nullptr,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    nullptr
                );

                if (hFile != INVALID_HANDLE_VALUE)
                {
                    CloseHandle(hFile);
                    return true; // 完全に開けたので完了
                }
            }
        }

        Sleep(500); // 0.5秒おきに確認
    }
    return false; // タイムアウト
}

bool theatria::FileSystem::ScriptProject::WaitForBuildNotification(int timeoutMs)
{
    HANDLE hPipe = CreateNamedPipeW(
        L"\\\\.\\pipe\\BuildWatcherPipe",
        PIPE_ACCESS_INBOUND,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1,
        0,
        0,
        timeoutMs,
        nullptr);

    if (hPipe == INVALID_HANDLE_VALUE)
    {
        std::wcerr << L"[WaitForBuildNotification] パイプ作成失敗: " << GetLastError() << std::endl;
        return false;
    }

    HANDLE hEvent = CreateEventW(NULL, TRUE, FALSE, L"TheatriaEngineReadyEvent");
    if (hEvent)
    {
        SetEvent(hEvent); // BuildWatcher に「準備できたよ」と通知
    }

    // BuildWatcher からの接続を待つ
    BOOL connected = ConnectNamedPipe(hPipe, nullptr) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
    if (!connected)
    {
        DWORD err = GetLastError();
		Log::Write(LogLevel::Error, "[WaitForBuildNotification] ConnectNamedPipe failed. err=" + std::to_string(err));
        CloseHandle(hPipe);
        return false;
    }

    wchar_t buffer[256];
    DWORD bytesRead = 0;
    bool buildSucceeded = false;

    // タイムアウト監視用
    DWORD startTick = GetTickCount();

    while (GetTickCount() - startTick < (DWORD)timeoutMs)
    {
        if (ReadFile(hPipe, buffer, sizeof(buffer) - sizeof(wchar_t), &bytesRead, nullptr) && bytesRead > 0)
        {
            buffer[bytesRead / sizeof(wchar_t)] = L'\0';
            std::wstring msg(buffer);

            if (msg == L"BUILD_START")
            {
                std::wcout << L"[通知] ビルド開始" << std::endl;
            }
            else if (msg.rfind(L"BUILD_FAIL:", 0) == 0)
            {
                std::wcout << L"[通知] 失敗: " << msg.substr(11) << std::endl;
                buildSucceeded = false;
            }
            else if (msg.rfind(L"BUILD_SUCCESS:", 0) == 0)
            {
                std::wcout << L"[通知] 成功: " << msg.substr(13) << std::endl;
                buildSucceeded = true;
            }
            else if (msg == L"BUILD_DONE")
            {
                std::wcout << L"[通知] ビルド完了" << std::endl;
                CloseHandle(hPipe);
                return buildSucceeded;
            }
        }
        Sleep(100);
    }
	if (hEvent) { CloseHandle(hEvent); }
    CloseHandle(hPipe);
    return false; // タイムアウト
}

bool theatria::FileSystem::ScriptProject::ConnectPipeToBuildWatcher(const int& timeoutMs)
{
	// パイプの作成
	// Read/Write それぞれ作成
    m_ReadPipe = CreateNamedPipeW(
		L"\\\\.\\pipe\\WatcherToEngine",
        PIPE_ACCESS_INBOUND,// 読み込み専用
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1,
        1024 * sizeof(wchar_t), // 出力バッファ
        1024 * sizeof(wchar_t), // 入力バッファ
        timeoutMs,
		nullptr);

    if (m_ReadPipe == INVALID_HANDLE_VALUE)
    {
        std::wcerr << L"[WatcherToEngine] パイプ作成失敗: " << GetLastError() << std::endl;
        return false;
	}

    m_WritePipe = CreateNamedPipeW(
		L"\\\\.\\pipe\\EngineToWatcher",
        PIPE_ACCESS_OUTBOUND,// 書き込み専用
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1,
        1024 * sizeof(wchar_t), // 出力バッファ
        1024 * sizeof(wchar_t), // 入力バッファ
		timeoutMs,
		nullptr);

    if (m_WritePipe == INVALID_HANDLE_VALUE)
    {
        std::wcerr << L"[EngineToWatcher] パイプ作成失敗: " << GetLastError() << std::endl;
        return false;
	}

    HANDLE hEvent = CreateEventW(NULL, TRUE, FALSE, L"TheatriaEngineReadyEvent");
    if (hEvent)
    {
        SetEvent(hEvent); // BuildWatcher に「準備できたよ」と通知
    }

    // BuildWatcher からの接続を待つ
    BOOL connected = ConnectNamedPipe(m_ReadPipe, nullptr) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
    if (!connected)
    {
        DWORD err = GetLastError();
        Log::Write(LogLevel::Error, "[WaitForBuildNotification] ConnectNamedPipe failed. err=" + std::to_string(err));
        CloseHandle(m_ReadPipe);
        return false;
    }
	BOOL connected2 = ConnectNamedPipe(m_WritePipe, nullptr) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
    if (!connected2)
    {
        DWORD err = GetLastError();
        Log::Write(LogLevel::Error, "[WaitForBuildNotification] ConnectNamedPipe failed. err=" + std::to_string(err));
        CloseHandle(m_WritePipe);
        return false;
    }

	// イベントハンドルはもう不要
    if (hEvent) { CloseHandle(hEvent); }

	return connected && connected2;
}

// メッセージ送信
void theatria::FileSystem::ScriptProject::SendMessageToBuildWatcher(const std::wstring& msg)
{
    if (m_WritePipe == INVALID_HANDLE_VALUE) return;

    std::wstring sendMsg = msg + L"\n"; // 改行を付ける（C#側 ReadLine 用）
    DWORD written = 0;
    BOOL ok = WriteFile(
        m_WritePipe,
        sendMsg.c_str(),
        (DWORD)(sendMsg.size() * sizeof(wchar_t)),
        &written,
        nullptr);

    FlushFileBuffers(m_WritePipe);

    if (!ok)
    {
        DWORD err = GetLastError();
		Log::Write(LogLevel::Info, "[Engine] WriteFile failed. err=" + std::to_string(err));
    }
	m_IsAttached = false;
}

// メッセージ受信
std::wstring theatria::FileSystem::ScriptProject::WaitForAckFromBuildWatcher(DWORD timeoutMs)
{
    if (m_ReadPipe == INVALID_HANDLE_VALUE) return L"";

    wchar_t buffer[256];
    DWORD bytesRead = 0;

    // --- Step 1: パイプ内の古いメッセージを破棄する ---
    DWORD bytesAvailable = 0;
    while (PeekNamedPipe(m_ReadPipe, nullptr, 0, nullptr, &bytesAvailable, nullptr) && bytesAvailable > 0)
    {
        // 余っている分を全部読み捨て
        BOOL b = ReadFile(m_ReadPipe, buffer, sizeof(buffer) - sizeof(wchar_t), &bytesRead, nullptr);
        b;
    }

    // --- Step 2: タイムアウト付きで新しいメッセージを待つ ---
    ULONGLONG start = GetTickCount64();
    while (GetTickCount64() - start < timeoutMs)
    {
        bytesAvailable = 0;
        if (PeekNamedPipe(m_ReadPipe, nullptr, 0, nullptr, &bytesAvailable, nullptr) && bytesAvailable > 0)
        {
            if (ReadFile(m_ReadPipe, buffer, sizeof(buffer) - sizeof(wchar_t), &bytesRead, nullptr) && bytesRead > 0)
            {
                size_t charCount = bytesRead / sizeof(wchar_t);
                buffer[charCount] = L'\0';
                std::wstring msg(buffer, charCount);

                // 改行を除去
                while (!msg.empty() && (msg.back() == L'\r' || msg.back() == L'\n'))
                    msg.pop_back();

                // --- Step 3: ACK判定 ---
                if (msg.rfind(L"ACK:", 0) == 0)
                {
                    return msg; // 期待通りのACKを受け取った
                }
                else
                {
                    // ACKじゃない場合はログを出して再ループ
                    Log::Write(LogLevel::Warn, L"Unexpected message: " + msg);
                }
            }
        }
        Sleep(10); // CPU負荷を抑える
    }

    // --- Step 4: タイムアウト ---
    Log::Write(LogLevel::Error, "Timeout waiting for ACK");
    return L"";
}

bool theatria::FileSystem::ScriptProject::TestPipeMessage()
{
    SendMessageToBuildWatcher(L"TEST_MESSAGE");
    // ここで必ず返事が来るまでブロック
    std::wstring reply = WaitForAckFromBuildWatcher(5000); // 5秒待機
    if (!reply.empty())
    {
        Log::Write(LogLevel::Info, L"Received from BuildWatcher: " + reply);
        return true;
    }
    return false;
}

bool theatria::FileSystem::ScriptProject::SaveAndBuildSolution(const bool& isBuild, const bool& isDebugger)
{
    bool buildSuccess = false;
    // .slnがついていなければ足す
    std::wstring slnPath = m_sProjectFolderPath + L"\\" + m_sProjectName;
    if (slnPath.size() < 4 || slnPath.substr(slnPath.size() - 4) != L".sln")
    {
        slnPath += L".sln";
    }
    /*HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE)
    {
        Log::Write(LogLevel::Assert, "CoInitializeEx failed", hr);
        return false;
    }*/

    CComPtr<IRunningObjectTable> rot;
    if (FAILED(GetRunningObjectTable(0, &rot))) { CoUninitialize(); return false; }

    CComPtr<IEnumMoniker> enumMoniker;
    if (FAILED(rot->EnumRunning(&enumMoniker))) { CoUninitialize(); return false; }

    CComPtr<IMoniker> moniker;
    while (enumMoniker->Next(1, &moniker, nullptr) == S_OK)
    {
        CComPtr<IBindCtx> bindCtx;
        HRESULT hr = CreateBindCtx(0, &bindCtx);
        hr;

        LPOLESTR displayName = nullptr;
        if (SUCCEEDED(moniker->GetDisplayName(bindCtx, nullptr, &displayName)))
        {
            std::wstring name(displayName);
            CoTaskMemFree(displayName);

            if (name.find(L"VisualStudio.DTE.") != std::wstring::npos)
            {
                CComPtr<IUnknown> unk;
                if (SUCCEEDED(rot->GetObject(moniker, &unk)))
                {
                    CComPtr<IDispatch> dte;
                    if (SUCCEEDED(unk->QueryInterface(IID_IDispatch, (void**)&dte)))
                    {
                        // Solution.FullName を取得
                        DISPID dispidSolution;
                        LPOLESTR solutionNamePtr = const_cast<LPOLESTR>(L"Solution");// const → 非constに変更
                        if (SUCCEEDED(dte->GetIDsOfNames(IID_NULL, &solutionNamePtr, 1, LOCALE_USER_DEFAULT, &dispidSolution)))
                        {
                            CComVariant result;
                            DISPPARAMS noArgs = { nullptr, nullptr, 0, 0 };
                            if (SUCCEEDED(dte->Invoke(dispidSolution, IID_NULL, LOCALE_USER_DEFAULT,
                                DISPATCH_PROPERTYGET, &noArgs, &result, nullptr, nullptr)))
                            {
                                if (result.vt == VT_DISPATCH && result.pdispVal)
                                {
                                    CComPtr<IDispatch> solution = result.pdispVal;
                                    DISPID dispidFullName;
									LPOLESTR fullNamePtr = const_cast<LPOLESTR>(L"FullName"); // const → 非constに変更
                                    if (SUCCEEDED(solution->GetIDsOfNames(IID_NULL, &fullNamePtr, 1, LOCALE_USER_DEFAULT, &dispidFullName)))
                                    {
                                        CComVariant solPath;
                                        if (SUCCEEDED(solution->Invoke(dispidFullName, IID_NULL, LOCALE_USER_DEFAULT,
                                            DISPATCH_PROPERTYGET, &noArgs, &solPath, nullptr, nullptr)))
                                        {
                                            if (solPath.vt == VT_BSTR)
                                            {
                                                bool hit = false;
                                                std::wstring fullPath = solPath.bstrVal;
                                                if (iequals(fullPath, slnPath))
                                                {
                                                    // 完全一致（推奨）
                                                    hit = true;
                                                }
                                                else
                                                {
                                                    // ファイル名だけ一致でもOKにする場合
                                                    auto fnA = std::filesystem::path(fullPath).filename().wstring();
                                                    auto fnB = std::filesystem::path(slnPath).filename().wstring();
                                                    std::transform(fnA.begin(), fnA.end(), fnA.begin(), ::towlower);
                                                    std::transform(fnB.begin(), fnB.end(), fnB.begin(), ::towlower);
                                                    if (fnA == fnB) hit = true;
                                                }

                                                // ★ slnPath に一致するかチェック（部分一致 or 完全一致）
                                                if (hit)
                                                {
                                                    Log::Write(LogLevel::Info, L"Target Solution Found: " + fullPath);

                                                    // SaveAll
                                                    VARIANT args[2];
                                                    VariantInit(&args[0]); VariantInit(&args[1]);
                                                    args[0].vt = VT_BSTR; args[0].bstrVal = SysAllocString(L"");
                                                    args[1].vt = VT_BSTR; args[1].bstrVal = SysAllocString(L"File.SaveAll");
                                                    InvokeByName(dte, L"ExecuteCommand", args, 2);
                                                    VariantClear(&args[0]); VariantClear(&args[1]);

                                                    if (isBuild)
                                                    {
                                                        // Build Solution
                                                        /*
                                                        VariantInit(&args[0]); VariantInit(&args[1]);
                                                        args[0].vt = VT_BSTR; args[0].bstrVal = SysAllocString(L"");
                                                        args[1].vt = VT_BSTR; args[1].bstrVal = SysAllocString(L"Build.BuildSolution");
                                                        if (SUCCEEDED(InvokeByName(dte, L"ExecuteCommand", args, 2)))
                                                        {
															any = WaitForBuildNotification(60000); // 60秒待つ
                                                        }
                                                        VariantClear(&args[0]); VariantClear(&args[1]);
                                                        */
														// Buildwatchr にビルド開始を通知
                                                        std::wstring config;
#ifdef _DEBUG
														config = L"Debug";
#elif NDEBUG
														config = L"Release";
#endif
                                                        if (isDebugger)
                                                        {
                                                            SendMessageToBuildWatcher(L"BUILD_SLN_DEBUGGER|" + m_sProjectName + L"|" + config + L"|x64");
                                                        }
                                                        else
                                                        {
                                                            SendMessageToBuildWatcher(L"BUILD_SLN|" + m_sProjectName + L"|" + config + L"|x64");
                                                        }
                                                        // ここで必ず返事が来るまでブロック
                                                        std::wstring reply = WaitForAckFromBuildWatcher(5000); // 5秒待機
                                                        if (!reply.empty())
                                                        {
                                                            Log::Write(LogLevel::Info, L"Received from BuildWatcher: " + reply);
                                                        }
                                                        if (reply == L"ACK:BUILD_SLN|OK")
                                                        {
                                                            buildSuccess = true;
                                                        }else if(reply == L"ACK:BUILD_SLN|FAIL")
                                                        {
                                                            buildSuccess = false;
                                                        }
                                                        m_IsAttached = isDebugger;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        moniker.Release();
    }

    // CoUninitialize();
    return buildSuccess;
}

bool theatria::FileSystem::ScriptProject::AddScriptFileToProject(const std::wstring& scriptName)
{
    // ADD_SCRIPT_PROJ|<sln>|<proj>|<filter>|<ClassName>
    std::wstring cmd = L"ADD_FILES_PROJ|Theatria|GameScript|Source Files|"
        + scriptName + L".cpp";
    SendMessageToBuildWatcher(cmd);

    // HLSL など相対パスも OK（プロジェクト直下からの相対）
    std::wstring cmd2 = L"ADD_FILES_PROJ|Theatria|GameScript|Shaders|Shaders\\DefaultPS.hlsl";
    SendMessageToBuildWatcher(cmd2);

    auto reply = WaitForAckFromBuildWatcher(5000);
    return reply.rfind(L"ACK:ADD_FILES|OK|", 0) == 0;
}

bool theatria::FileSystem::ScriptProject::AddClassFileToProject(const std::wstring& className)
{
    // .h は Header Files, .cpp は Source Files に自動で入る
    SendMessageToBuildWatcher(L"ADD_SCRIPT_PROJ|"+ m_sProjectName + L"|" + m_sProjectName + L"|Source Files|" + className);
    auto reply = WaitForAckFromBuildWatcher(5000);
    return reply.rfind(L"ACK:ADD_SCRIPT|OK|", 0) == 0;
}

std::wstring theatria::FileSystem::ScriptProject::norm_path(const std::wstring& p)
{
    wchar_t buf[MAX_PATH];
    DWORD n = GetFullPathNameW(p.c_str(), MAX_PATH, buf, nullptr);
    std::wstring s = (n && n < MAX_PATH) ? buf : p;
    std::replace(s.begin(), s.end(), L'/', L'\\');
    std::transform(s.begin(), s.end(), s.begin(),
        [](wchar_t c) { return static_cast<wchar_t>(std::tolower(c)); });
    return s;
}

bool theatria::FileSystem::ScriptProject::iequals(const std::wstring& a, const std::wstring& b)
{
    return norm_path(a) == norm_path(b);
}

std::wstring theatria::FileSystem::ScriptProject::NowStamp()
{
    using namespace std::chrono;

    const auto tp = system_clock::now();
    const std::time_t tt = system_clock::to_time_t(tp);

    // 安全版：呼び出し側バッファに書き込む
    std::tm lt{};
    localtime_s(&lt, &tt);

    // マイクロ秒（0～999999）
    const auto usec =
        duration_cast<microseconds>(tp.time_since_epoch()).count() % 1'000'000;

    wchar_t buf[64];
    // 安全版フォーマット
    swprintf_s(buf, sizeof(buf) / sizeof(buf[0]),
        L"%04d%02d%02d-%02d%02d%02d-%06lld",
        lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday,
        lt.tm_hour, lt.tm_min, lt.tm_sec,
        static_cast<long long>(usec));

    return std::wstring(buf);
}

bool theatria::FileSystem::ScriptProject::CopyWithRetry(const fs::path& src, const fs::path& dst, int retries, DWORD waitMs, bool overwrite)
{
    auto opts = overwrite ? fs::copy_options::overwrite_existing
        : fs::copy_options::none;

    std::error_code ec;
    for (int i = 0; i < retries; ++i)
    {
        ec.clear();
        fs::copy_file(src, dst, opts, ec);
        if (!ec) return true;

        // 共有違反・ロックっぽいエラーはリトライ
        DWORD winerr = (DWORD)ec.value();
        if (winerr == ERROR_SHARING_VIOLATION || winerr == ERROR_LOCK_VIOLATION)
        {
            ::Sleep(waitMs);
            continue;
        }
        // それ以外は即失敗
        break;
    }
    return false;
}

theatria::FileSystem::ScriptProject::StagedFiles theatria::FileSystem::ScriptProject::StageDllAndPdbInSiblingFolder(const fs::path& originalDll, const std::wstring& stageRootName)
{
    StagedFiles out{};
    out.originalDll = originalDll;

    // PDB は DLL と同名（拡張子 .pdb）を隣で探す
    fs::path pdb = originalDll;
    pdb.replace_extension(L".pdb");
    if (fs::exists(pdb)) out.originalPdb = pdb;

    // 親フォルダ/.stage/<pid>-<timestamp>
    fs::path parent = originalDll.parent_path();
    DWORD pid = ::GetCurrentProcessId();
    fs::path stageRoot = parent / stageRootName;
    fs::path stageDir = stageRoot / (std::to_wstring(pid) + L"-" + NowStamp());

    std::error_code ec;
    fs::create_directories(stageDir, ec);
    if (ec)
    {
        // ディレクトリ作成失敗
        return out; // stagedDll==空 で失敗を示す
    }

    // DLLコピー
    fs::path stagedDll = stageDir / originalDll.filename();
    if (!CopyWithRetry(originalDll, stagedDll))
    {
        // 失敗したらディレクトリ掃除だけ試みる
        std::error_code ec2; fs::remove_all(stageDir, ec2);
        return out;
    }

    // PDBコピー（存在していれば）
    fs::path stagedPdb;
    if (!out.originalPdb.empty())
    {
        stagedPdb = stageDir / out.originalPdb.filename();
        // PDBコピーは失敗しても致命ではないが、戻り値に反映したいので試す
        CopyWithRetry(out.originalPdb, stagedPdb);
    }

    out.stageDir = stageDir;
    out.stagedDll = stagedDll;
    if (!stagedPdb.empty() && fs::exists(stagedPdb)) out.stagedPdb = stagedPdb;
    return out;
}

void theatria::FileSystem::ScriptProject::CleanupStage(const StagedFiles& staged)
{
    if (staged.stageDir.empty()) return;
    std::error_code ec; fs::remove_all(staged.stageDir, ec);
}

// Pipe
void theatria::FileSystem::ScriptProject::ClosePipe()
{
	CloseHandle(m_ReadPipe);
	CloseHandle(m_WritePipe);
}



void theatria::Deserialization::FromJson(const json& j, TransformComponent& t)
{
	t.position = { j["translation"][0], j["translation"][1], j["translation"][2] };
	t.quaternion = { j["rotation"][0], j["rotation"][1], j["rotation"][2], j["rotation"][3] };
	t.scale = { j["scale"][0], j["scale"][1], j["scale"][2] };
	t.degrees = { j["degrees"][0], j["degrees"][1], j["degrees"][2] };
	t.isBillboard = j.value("isBillboard", false);
}

void theatria::Deserialization::FromJson(const json& j, CameraComponent& c)
{
	c.fovAngleY = j.value("fovAngleY", 45.0f);
	c.aspectRatio = j.value("aspectRatio", 1.777f);
	c.nearZ = j.value("nearZ", 0.1f);
	c.farZ = j.value("farZ", 1000.0f);
}

void theatria::Deserialization::FromJson(const json& j, MeshFilterComponent& m)
{
    j;m;
	/*if (j.contains("modelName"))
	{
		m.modelName = j["modelName"].get<std::string>();
		m.modelID = j["modelID"].get<std::string>();
	} else
	{
		m.modelName = "";
		m.modelID.reset();
	}*/
}

void theatria::Deserialization::FromJson(const json& j, MeshRendererComponent& r)
{
	r.visible = j.value("visible", true);
}

void theatria::Deserialization::FromJson(const json& j, MaterialComponent& m)
{
	m.color = { j["Color"][0], j["Color"][1], j["Color"][2], j["Color"][3] };
	m.textureName = ConvertString(j.value("textureName",""));
	m.enableLighting = j.value("enableLighting", true);
	m.enableTexture = j.value("enableTexture",false);
	m.shininess = j.value("shininess", 50.0f);
	m.uvFlipY = j.value("uvFlipY", false);
}

void theatria::Deserialization::FromJson(const json& j, ScriptComponent& s, ScriptContainer* container)
{
    // scriptName
    if (j.contains("scriptName"))
    {
        s.scriptName = j.at("scriptName").get<std::string>();
    }
    else if (j.contains("Script") && j["Script"].contains("scriptName"))
    {
        s.scriptName = j["Script"]["scriptName"].get<std::string>();
    }
    container->AddScriptData(s.scriptName);
    ScriptData* sd = container->GetScriptDataByName(s.scriptName);
    if (!j.contains("fields") || !j.at("fields").is_object()) return;

    const json& jf = j.at("fields");

    for (auto it = jf.begin(); it != jf.end(); ++it)
    {
        const std::string key = it.key();
        const json& obj = it.value();
        if (!obj.is_object()) continue;

        // 必須: value / type
        if (!obj.contains("value") || !obj.contains("type")) continue;

        const json& jval = obj.at("value");
        const std::string typeStr = obj.at("type").get<std::string>();

        ScriptComponent::FieldVal saved;

        // 任意: minmax（float/intのみ有効）
        if (obj.contains("minmax"))
        {
            const json& mm = obj.at("minmax");
            if (mm.is_array() && mm.size() == 2)
            {
                saved.minmax.first = mm[0].get<uint32_t>();
                saved.minmax.second = mm[1].get<uint32_t>();
            }
        }

        // 値の復元
        if (typeStr == "float")
        {
            saved.type = typeid(float);
            float v = jval.get<float>();
            // クランプ（必要なければこの2行を外してOK）
            if (saved.minmax.first != 0 || saved.minmax.second != 0)
            {
                v = std::clamp(v, static_cast<float>(saved.minmax.first),
                    static_cast<float>(saved.minmax.second));
            }
            saved.value = v;
        }
        else if (typeStr == "int")
        {
            saved.type = typeid(int);
            int v = jval.get<int>();
            if (saved.minmax.first != 0 || saved.minmax.second != 0)
            {
                v = std::clamp(v, static_cast<int>(saved.minmax.first),
                    static_cast<int>(saved.minmax.second));
            }
            saved.value = v;
        }
        else if (typeStr == "bool")
        {
            saved.type = typeid(bool);
            saved.value = jval.get<bool>();
        }
        else if (typeStr == "float3")
        {
            float3 v{};
            if (jval.is_array() && jval.size() >= 3)
            {
                v.x = jval[0].get<float>();
                v.y = jval[1].get<float>();
                v.z = jval[2].get<float>();
            }
            else if (jval.is_object() && jval.contains("x") && jval.contains("y") && jval.contains("z"))
            {
                // 念のため { "x":..., "y":..., "z":... } 形式にも対応
                v.x = jval["x"].get<float>();
                v.y = jval["y"].get<float>();
                v.z = jval["z"].get<float>();
            }
            else
            {
                continue; // 不正
            }
            saved.type = typeid(float3);
            saved.value = v;
        }
        else
        {
            // 未知の type はスキップ（必要ならここで例外やログ）
            continue;
        }

        sd->saveFields[key] = std::move(saved);
    }
}

void theatria::Deserialization::FromJson(const json& j, std::vector<LineRendererComponent>& ls)
{
    j;ls;
	/*if (j.is_array())
	{
		for (const auto& item : j)
		{
			LineRendererComponent l;
			l.line.start = { item["start"][0], item["start"][1], item["start"][2] };
			l.line.end = { item["end"][0], item["end"][1], item["end"][2] };
			l.line.color = { item["color"][0], item["color"][1], item["color"][2], item["color"][3] };
			ls.push_back(l);
		}
	}*/
}

void theatria::Deserialization::FromJson(const json& j, Rigidbody2DComponent& rb)
{
    j;rb;
	/*rb.isKinematic = j.value("isKinematic", false);
	rb.gravityScale = j.value("gravityScale", 1.0f);
	rb.mass = j.value("mass", 1.0f);
	rb.bodyType = static_cast<Rigidbody2DComponent::BodyType>(j.value("bodyType", 0));
	rb.fixedRotation = j.value("fixedRotation", false);*/
}

void theatria::Deserialization::FromJson(const json& j, BoxCollider2DComponent& bc)
{
	bc.offsetX = j.value("offsetX", 0.0f);
	bc.offsetY = j.value("offsetY", 0.0f);
	bc.width = j.value("width", 1.0f);
	bc.height = j.value("height", 1.0f);
	bc.density = j.value("density", 1.0f);
	bc.friction = j.value("friction", 0.5f);
	bc.restitution = j.value("restitution", 0.0f);
}

void theatria::Deserialization::FromJson(const json& j, Rigidbody3DComponent& rb)
{
	rb.SetActive(j.value("active", true));
	rb.friction = j.value("friction", 0.5f);
	rb.restitution = j.value("restitution", 0.0f);
	rb.fixedPositionX = j.value("fixedPositionX", false);
	rb.fixedPositionY = j.value("fixedPositionY", false);
	rb.fixedPositionZ = j.value("fixedPositionZ", false);
	rb.fixedRotationX = j.value("fixedRotationX", false);
	rb.fixedRotationY = j.value("fixedRotationY", false);
	rb.fixedRotationZ = j.value("fixedRotationZ", false);
	rb.halfsize = { j["halfsize"][0], j["halfsize"][1], j["halfsize"][2] };
	rb.bodyType = static_cast<physics::d3::Id3BodyType>(j.value("bodyType", 0));
	rb.gravityScale = j.value("gravityScale", 1.0f);
	rb.mass = j.value("mass", 1.0f);
	rb.isSensor = j.value("isSensor", false);
    if (j.contains("velocity") && j["velocity"].is_array() && j["velocity"].size() == 3)
    {
        rb.velocity = { j["velocity"][0], j["velocity"][1], j["velocity"][2] };
    }
    else
    {
        rb.velocity = { 0.0f, 0.0f, 0.0f }; // デフォルト
    }
    if(j.contains("quaternion") && j["quaternion"].is_array() && j["quaternion"].size() == 4)
    {
        rb.quaternion = { j["quaternion"][0], j["quaternion"][1], j["quaternion"][2], j["quaternion"][3] };
    }
    else
    {
        rb.quaternion = { 0.0f, 0.0f, 0.0f, 1.0f }; // デフォルト
	}
}

void theatria::Deserialization::FromJson(const json& j, EmitterComponent& e)
{
    // lifeTime
    if (j.contains("lifeTime") && j["lifeTime"].is_array() && j["lifeTime"].size() == 2)
    {
        e.lifeTime.median = j["lifeTime"][0].get<float>();
        e.lifeTime.amplitude = j["lifeTime"][1].get<float>();
    }

    auto ReadRandValue = [](const json& jv, RandValue& out) {
        if (jv.is_array() && jv.size() == 2)
        {
            out.median = jv[0].get<float>();
            out.amplitude = jv[1].get<float>();
        }
        };

    auto ReadRandfloat3Array = [&](const json& jarr, Randfloat3& out) {
        if (jarr.is_array() && jarr.size() == 3)
        {
            if (jarr[0].contains("x")) ReadRandValue(jarr[0]["x"], out.x);
            if (jarr[1].contains("y")) ReadRandValue(jarr[1]["y"], out.y);
            if (jarr[2].contains("z")) ReadRandValue(jarr[2]["z"], out.z);
        }
        };

    auto ReadPVAArray = [&](const json& jpva, PVA& out) {
        if (jpva.contains("value")) ReadRandfloat3Array(jpva["value"], out.value);
        if (jpva.contains("velocity")) ReadRandfloat3Array(jpva["velocity"], out.velocity);
        if (jpva.contains("acceleration")) ReadRandfloat3Array(jpva["acceleration"], out.acceleration);
        };

    if (j.contains("position"))
    {
        ReadPVAArray(j["position"], e.position);
    }
    if (j.contains("rotation"))
    {
        ReadPVAArray(j["rotation"], e.rotation);
    }
    if (j.contains("scale"))
    {
        ReadPVAArray(j["scale"], e.scale);
    }

    e.frequency = j.value("frequency", 0.1f);
    e.frequencyTime = j.value("frequencyTime", 0.1f);
    e.emitCount = j.value("emitCount", 1);
    e.isFadeOut = j.value("isFadeOut", false);
    e.isBillboard = j.value("isBillboard", true);
}

void theatria::Deserialization::FromJson(const json& j, ParticleComponent& p)
{
	p.count = j.value("count", 1024);
}

void theatria::Deserialization::FromJson(const json& j, UISpriteComponent& ui)
{
	ui.position = { j["position"][0], j["position"][1] };
	ui.rotation = j.value("rotation", 0.0f);
	ui.scale = { j["scale"][0], j["scale"][1] };
	ui.anchorPoint = { j["anchorPoint"][0], j["anchorPoint"][1] };
	ui.size = { j["size"][0], j["size"][1] };
	ui.textureLeftTop = { j["textureLeftTop"][0], j["textureLeftTop"][1] };
	ui.textureSize = { j["textureSize"][0], j["textureSize"][1] };
}

void theatria::Deserialization::FromJson(const json& j, LightComponent& l)
{
	l.color = { j["color"][0], j["color"][1], j["color"][2], j["color"][3] };
	l.intensity = j.value("intensity", 1.0f);
	l.range = j.value("range", 10.0f);
	l.decay = j.value("decay", 1.0f);
	l.spotAngle = j.value("spotAngle", 45.0f);
	l.spotFalloffStart = j.value("spotFalloffStart", 0.0f);
	l.type = static_cast<LightType>(j.value("type", 0));
	l.active = j.value("active", true);
}

void theatria::Deserialization::FromJson(const json& j, AudioComponent& a)
{
	//a.audioName = j.value("audioName", "");
	a.isLoop = j.value("isLoop", false);
}

void theatria::Deserialization::FromJson(const json& j, AnimationComponent& a)
{
	a.transitionDuration = j.value("transitionDuration", 0.2f);
	a.animationIndex = j.value("animationIndex", 0);
	std::string modelName = j.value("modelName", "");
	a.modelName = ConvertString(modelName);
}

void theatria::FileSystem::ScanFolder(const path& rootPath, EngineCommand* engineCommand)
{
    g_ProjectFiles = ScanRecursive(rootPath,engineCommand);
}

FolderNode theatria::FileSystem::ScanRecursive(const path& path, EngineCommand* engineCommand)
{
    FolderNode node;        // 一番上のノード
	node.folderPath = path; // フォルダパス

    // フォルダの中を走査
    for (const auto& entry : directory_iterator(path))
    {
		// フォルダなら再帰処理
        if (entry.is_directory())
        {
			// フォルダ名がbin,obj,.vs,.gitの場合はスキップ
            if (entry.path().filename() == "bin" ||
                entry.path().filename() == "obj" ||
                entry.path().filename() == ".vs" ||
                entry.path().filename() == ".git")
            {
                continue;
            }
			node.children.push_back(ScanRecursive(entry.path(),engineCommand));
        }
		// ファイルなら処理
		else if (entry.is_regular_file())
		{
            if (ProcessFile(entry.path(),engineCommand))// 処理に成功したらノードに追加
            {
                node.files.push_back(entry.path());
            }
		}
    }
    return node;
}

// フォルダノードをパスで検索
FolderNode* theatria::FileSystem::FindFolderNodeByPath(FolderNode& node, const std::filesystem::path& target)
{
    if (node.folderPath == target)
        return &node;

    for (auto& child : node.children)
    {
        if (FolderNode* found = FindFolderNodeByPath(child, target))
            return found;
    }
    return nullptr;
}

bool theatria::FileSystem::ProcessFile(const path& filePath, EngineCommand* engineCommand)
{
	std::wstring wFileName = filePath.filename().wstring();
	std::wstring scriptName = wFileName.substr(0, wFileName.find_last_of('.')); // 拡張子を除いたファイル名

    // テクスチャファイル
    if (wFileName.ends_with(L".dds")|| wFileName.ends_with(L".png") || wFileName.ends_with(L".jpg"))
    {
		// テクスチャの処理
        engineCommand->GetResourceManager()->GetTextureManager()->LoadTextureFile(filePath);
        return true;
    }
	// モデルファイル
    if (wFileName.ends_with(L".fbx") || wFileName.ends_with(L".gltf") || wFileName.ends_with(L".obj"))
    {
		// モデルの処理
        return engineCommand->GetResourceManager()->GetModelManager()->LoadModelFile(filePath);
    }
	// 音声ファイル
	if (wFileName.ends_with(L".wav") || wFileName.ends_with(L".mp3"))
	{
		// 音声の処理
        return engineCommand->GetResourceManager()->GetAudioManager()->SoundLordWave(filePath.c_str());
	}
	// スクリプトファイル
	if (wFileName.ends_with(L".cpp") || wFileName.ends_with(L".h"))
	{
		// スクリプトの処理
        engineCommand->GetResourceManager()->GetScriptContainer()->AddScriptData(ConvertString(scriptName));
        return false;
	}
	// jsonファイル
	if (wFileName.ends_with(L".json"))
	{
		// jsonの処理
		FileType type = FileSystem::GetJsonFileType(filePath);

        switch (type)
        {
        case theatria::ChoProject:
            break;
        case theatria::EngineConfig:
            break;
        case theatria::GameSettings:
        {
            return LoadGameSettings(filePath);
        }
            break;
        case theatria::SceneFile:// シーンファイル
        {
			return LoadSceneFile(filePath,engineCommand);
        }
            break;
        case theatria::ModelFile:
            break;
        case theatria::ImageFile:
            break;
        case theatria::SoundFile:
            break;
        case theatria::EffectFile:
            break;
		case theatria::ScriptFile:// スクリプトファイル
        {
			//return LoadScriptFile(filePath, engineCommand);
        }
            break;
        case theatria::GameParameter:
            break;
        case theatria::PrefabFile:
            break;
        case theatria::Unknown:
            break;
        default:
            break;
        }
	}
	// その他のファイルは無視
	return false;
}

bool theatria::FileSystem::AddFile(const path& filePath, FolderNode& folderNode, EngineCommand* engineCommand)
{
    namespace fs = std::filesystem;
    if (filePath.empty() || !fs::exists(filePath)) return false;

    try
    {
        if (fs::is_regular_file(filePath))
        {
            path dstPath = folderNode.folderPath / filePath.filename();

            // コピー（上書き許可）
            if (!fs::copy_file(filePath, dstPath, fs::copy_options::overwrite_existing))
            {
				// コピー失敗
				return false;
            }

            // コピー後のパスで処理
            if (ProcessFile(dstPath, engineCommand))
            {
                folderNode.files.push_back(dstPath);
                return true;
            } else
            {
                fs::remove(dstPath); // 処理失敗なら削除
            }
        } else if (fs::is_directory(filePath))
        {
            path dstDir = folderNode.folderPath / filePath.filename();

            // ディレクトリ作成
            fs::create_directories(dstDir);

            FolderNode childNode;
            childNode.folderPath = dstDir;

            bool hasValid = false;

            for (const auto& entry : fs::directory_iterator(filePath))
            {
                if (AddFile(entry.path(), childNode, engineCommand))
                {
                    hasValid = true;
                }
            }

            if (hasValid)
            {
                folderNode.children.push_back(std::move(childNode));
                return true;
            } else
            {
                fs::remove_all(dstDir); // 有効なファイルがなければ削除
            }
        }
    }
    catch (const fs::filesystem_error& e)
    {
        // ログ出力などがあれば追加
        theatria::Log::Write(LogLevel::Assert,"AddFile failed: " + filePath.string() + " (" + e.what() + ")");
    }

    return false;
}

std::wstring theatria::FileSystem::GameBuilder::SelectFolderDialog()
{
    std::wstring selectedPath;
    HRESULT hr;

    IFileDialog* pFileDialog = nullptr;

    // IFileOpenDialogを作成
    hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&pFileDialog));

    if (SUCCEEDED(hr))
    {
        // フォルダ選択モードに設定
        DWORD dwOptions;
        pFileDialog->GetOptions(&dwOptions);
        pFileDialog->SetOptions(dwOptions | FOS_PICKFOLDERS);

        // ダイアログを表示
        hr = pFileDialog->Show(NULL);

        if (SUCCEEDED(hr))
        {
            IShellItem* pItem;
            hr = pFileDialog->GetResult(&pItem);
            if (SUCCEEDED(hr))
            {
                PWSTR pszFilePath;
                hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                if (SUCCEEDED(hr))
                {
                    selectedPath = pszFilePath;
                    CoTaskMemFree(pszFilePath);
                }
                pItem->Release();
            }
        }
        pFileDialog->Release();
    }
    return selectedPath;
}

void theatria::FileSystem::GameBuilder::CopyFilesToBuildFolder([[maybe_unused]]EngineCommand* engineCommand, const std::wstring& folderPath)
{
    namespace fs = std::filesystem;
    fs::path buildRoot = fs::path(folderPath) / m_sProjectName;
    try
    {
        if (!fs::exists(buildRoot))
        {
            fs::create_directories(buildRoot);
        }

        LaunchConfig config;
        config.projectName = m_sProjectName;
        SaveLaunchConfig(config, buildRoot);

        // コピーするパス一覧（構造維持したいパス）
        std::vector<fs::path> engineSources = {
            L"TheatriaEngine_GameRuntime.dll",
            L"TheatriaEngine_GameRuntime.exp",
            L"TheatriaEngine_GameRuntime.lib",
            //L"ChoMath.lib",
            L"dxcompiler_GameRuntime.dll",
            L"dxil_GameRuntime.dll",
            L"GameTemplate.exe",
            L"imgui.ini", // 後で消す
            L"Cho/Engine",
            L"Cho/Resources/EngineAssets",
            L"Cho/pch",
            L"Cho/APIExportsMacro.h",
            L"Cho/TheatriaEngineAPI.h",
        };
        std::vector<fs::path> gameSources = {
            fs::path(m_sProjectFolderPath) / fs::path(L"Assets"),
            fs::path(m_sProjectFolderPath) / fs::path(L"bin"),
            fs::path(m_sProjectFolderPath) / fs::path(L"ProjectSettings"),
        };

        /*for (const auto& scene : engineCommand->GetGameCore()->GetSceneManager()->GetScenes())
        {
			sources.push_back(fs::path(L"GameProjects") / m_sProjectName / fs::path(scene.GetName()).replace_extension(L".json"));
        }*/

        for (const auto& src : engineSources)
        {
            fs::path sourcePath = fs::absolute(src);

            if (!fs::exists(sourcePath))
            {
                theatria::Log::Write(LogLevel::Assert, "Source file or directory does not exist: " + sourcePath.string());
                continue;
            }

            // 構造を維持するコピー先パスを決定
            fs::path relativePath = fs::relative(sourcePath, fs::current_path());
            fs::path destinationPath = buildRoot / relativePath;

            // フォルダを作成
            fs::create_directories(destinationPath.parent_path());

            // コピー実行
            if (fs::is_directory(sourcePath))
            {
                fs::copy(sourcePath, destinationPath, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
            } else if (fs::is_regular_file(sourcePath))
            {
                fs::copy_file(sourcePath, destinationPath, fs::copy_options::overwrite_existing);
            }
        }
        for (const auto& src : gameSources)
        {
            fs::path sourcePath = src;

            if (!fs::exists(src))
            {
                theatria::Log::Write(LogLevel::Assert, "Source file or directory does not exist: " + sourcePath.string());
                continue;
            }

            // 構造を維持するコピー先パスを決定
            //fs::path relativePath = fs::relative(sourcePath, fs::current_path());
            fs::path destinationPath = buildRoot / src.filename();

            // フォルダを作成
            fs::create_directories(destinationPath.parent_path());

            // コピー実行
            if (fs::is_directory(sourcePath))
            {
                fs::copy(sourcePath, destinationPath, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
            }
            else if (fs::is_regular_file(sourcePath))
            {
                fs::copy_file(sourcePath, destinationPath, fs::copy_options::overwrite_existing);
            }
        }

        theatria::Log::Write(LogLevel::Info, "CopyFilesToBuildFolder completed successfully.");
    }
    catch (const std::exception& e)
    {
        theatria::Log::Write(LogLevel::Assert, "CopyFilesToBuildFolder failed: " + std::string(e.what()));
    }

    // exeの名前
    std::wstring finalExeName = g_GameSettings.exeName + L".exe"; // 任意の最終ファイル名

    std::vector<std::pair<fs::path, fs::path>> renameList = {
    { buildRoot / L"TheatriaEngine_GameRuntime.dll",    buildRoot / L"TheatriaEngine.dll" },
    { buildRoot / L"TheatriaEngine_GameRuntime.exp",    buildRoot / L"TheatriaEngine.exp" },
    { buildRoot / L"TheatriaEngine_GameRuntime.lib",    buildRoot / L"TheatriaEngine.lib" },
    { buildRoot / L"dxcompiler_GameRuntime.dll",   buildRoot / L"dxcompiler.dll" },
    { buildRoot / L"dxil_GameRuntime.dll",         buildRoot / L"dxil.dll" },
    { buildRoot / L"GameTemplate.exe",             buildRoot / finalExeName }, // 任意の最終ファイル名
    };

    for (const auto& [from, to] : renameList)
    {
        try
        {
            if (fs::exists(from))
            {
                fs::rename(from, to);
            } else
            {
                theatria::Log::Write(LogLevel::Assert, "File not found to rename: " + from.string());
            }
        }
        catch (const std::exception& e)
        {
            theatria::Log::Write(LogLevel::Assert, "Rename failed: " + from.string() + " → " + to.string() + " : " + e.what());
        }
    }

}

std::wstring theatria::FileSystem::GameBuilder::GetEnvVar(const wchar_t* name)
{
    wchar_t buffer[512];
    size_t len = 0;
    _wgetenv_s(&len, buffer, name);
    return std::wstring(buffer, len - 1);
}

