#pragma once
#define NOMINMAX // Windowのminmaxマクロを除外
#define _WIN32_DCOM
#include <Windows.h>
#include <sstream>
#include <regex>
#include <string>
#include <optional>
#include <fstream>
#include <filesystem>
#include <json.hpp>
#include "Core/Utility/Components.h"
using namespace std::filesystem;
using json = nlohmann::json;
using GameParameterVariant = std::variant<int, float, bool, float3>;
class GameScene;
class GameWorld;
class SceneManager;
class ECSManager;
class ResourceManager;
class EditorManager;
class EngineCommand;
class ScriptContainer;
namespace theatria
{
    enum FileType
    {
        ChoProject,     // プロジェクトファイル
		EngineConfig,   // エンジン設定ファイル
		GameSettings,   // ゲーム設定ファイル
		SceneFile,      // シーンファイル
		ModelFile,      // モデルファイル
		ImageFile,      // 画像ファイル
		SoundFile,      // 音声ファイル
		EffectFile,     // エフェクトファイル
		ScriptFile,     // スクリプトファイル
		GameParameter,  // ゲームパラメータファイル
		PrefabFile,     // プレハブファイル
		Unknown,        // 不明なファイル
    };

    // 戻り値用構造体
    struct ProjectInfo
    {
        std::string name;
        std::string version;
        std::vector<std::wstring> scenes;
    };
    struct WindowConfigInfo
    {
        int width = 1280;
        int height = 720;
        bool fullscreen = false;
        std::wstring titleBar = L"Theatria Engine";
    };
    struct EngineConfigInfo
    {
        std::string renderer = "DirectX12";
        WindowConfigInfo window;
        bool vsync = true;
    };
    struct GameSettingsInfo
    {
        std::wstring startScene = L"MainScene";
        int frameRate = 60;
        float fixedDeltaTime = 1.0f / 60.0f;
        bool debugMode = false;
        std::wstring skyTexName;
		float3 gravity = { 0.0f, 0.0f, 0.0f }; // 重力
        std::wstring titleBar = L"Theatria Engine";
        std::wstring exeName = L"TheatriaGame";
    };
    struct FolderNode
    {
		std::filesystem::path folderPath;
		std::vector<std::filesystem::path> files;
		std::vector<FolderNode> children;
    };

    // 起動Configファイル
    struct LaunchConfig
    {
        std::wstring projectName = L"";
    };

    // キャッシュファイル構成
    struct CacheFile
    {
        std::vector<std::wstring> projectNames;
    };

    // ComponentsSerializer
    namespace Serialization
    {
        // コンポーネントを保存
        json ToJson(const TransformComponent& t);
        json ToJson(const CameraComponent& c);
        json ToJson(const MeshFilterComponent& m);
        json ToJson(const MeshRendererComponent& r);
		json ToJson(const MaterialComponent& m);
		json ToJson(const ScriptComponent& s, ScriptContainer* container);
		json ToJson(const std::vector<LineRendererComponent>& ls);
		json ToJson(const Rigidbody2DComponent& rb);
		json ToJson(const BoxCollider2DComponent& bc);
		json ToJson(const Rigidbody3DComponent& rb);
		json ToJson(const EmitterComponent& e);
		json ToJson(const ParticleComponent& p);
		json ToJson(const UISpriteComponent& ui);
		json ToJson(const LightComponent& l);
		json ToJson(const AudioComponent& a);
		json ToJson(const AnimationComponent& a);
    }
	// ComponentsDeserializer
	namespace Deserialization
	{
		// コンポーネントを読み込み
		void FromJson(const json& j, TransformComponent& t);
		void FromJson(const json& j, CameraComponent& c);
		void FromJson(const json& j, MeshFilterComponent& m);
		void FromJson(const json& j, MeshRendererComponent& r);
		void FromJson(const json& j, MaterialComponent& m);
		void FromJson(const json& j, ScriptComponent& s, ScriptContainer* container);
		void FromJson(const json& j, std::vector<LineRendererComponent>& ls);
		void FromJson(const json& j, Rigidbody2DComponent& rb);
		void FromJson(const json& j, BoxCollider2DComponent& bc);
		void FromJson(const json& j, Rigidbody3DComponent& rb);
		void FromJson(const json& j, EmitterComponent& e);
		void FromJson(const json& j, ParticleComponent& p);
		void FromJson(const json& j, UISpriteComponent& ui);
		void FromJson(const json& j, LightComponent& l);
		void FromJson(const json& j, AudioComponent& a);
		void FromJson(const json& j, AnimationComponent& a);
	}

    class FileSystem
    {
    public:
        // フォルダ走査
        static void ScanFolder(const path& rootPath,EngineCommand* engineCommand);
        static FolderNode ScanRecursive(const path& path, EngineCommand* engineCommand);
		// フォルダノードをパスで検索
        static FolderNode* FindFolderNodeByPath(FolderNode& node, const std::filesystem::path& target);
        // 拡張子ごとに処理を分ける関数
        static bool ProcessFile(const path& filePath, EngineCommand* engineCommand);
        // ファイル、フォルダを追加、処理
		static bool AddFile(const path& filePath, FolderNode& folderNode,EngineCommand* engineCommand);
		// プロジェクトフォルダを探す
        static std::optional<std::filesystem::path> FindOrCreateGameProjects();
		// プロジェクトフォルダを取得
        static std::vector<std::wstring> GetProjectFolders();
        // 新しいプロジェクトを作成
        static bool CreateNewProjectFolder(const std::wstring& projectName, const std::wstring& projectPath);
		// プロジェクトファイルを保存
        static bool SaveProjectFile(const std::wstring& projectName, const std::vector<std::wstring>& sceneFiles);
		// プロジェクトファイルを読み込む
        static std::optional<theatria::ProjectInfo> LoadProjectFile(const std::wstring& projectName);
		// エンジン設定ファイルを保存
        static bool SaveEngineConfig(const std::wstring& projectName, const EngineConfigInfo& config);
		// エンジン設定ファイルを読み込む
        static std::optional<EngineConfigInfo> LoadEngineConfig(const std::wstring& projectName);
		// ゲーム設定ファイルを保存
        static bool SaveGameSettings(const std::wstring& projectName, const theatria::GameSettingsInfo& settings);
		// ゲーム設定ファイルを読み込む
        static bool LoadGameSettings(const std::wstring& filePath);
		// シーンファイルを保存
        static bool SaveSceneFile(const std::wstring& directory,const std::wstring& srcFileName, GameScene* scene, ECSManager* ecs,EngineCommand* engineCommand);
		// シーンファイルを読み込む
        static bool LoadSceneFile(const std::wstring& filePath,EngineCommand* engineCommand);
        // ゲームパラメーターファイルを保存
        static bool SaveGameParameter(const std::wstring& filePath,
            const std::string& group,
            const std::string& item,
            const std::string& dataName,
            const GameParameterVariant& value);
		// ゲームパラメーターファイルを読み込む
        static bool LoadGameParameter(const std::wstring& filePath,
            const std::string& group,
            const std::string& item,
            const std::string& dataName,
            GameParameterVariant& outValue);
        // 起動Configファイルを保存
        static bool SaveLaunchConfig(const LaunchConfig& config, const std::wstring& filePath);
        // 起動Configファイルを読み込む
        static LaunchConfig LoadLaunchConfig(const std::wstring& filePath);
        // キャッシュファイルを保存
        static bool SaveCacheFile(const CacheFile& cache, const std::wstring& filePath);
        // キャッシュファイルを読み込む
        static bool LoadCacheFile(const std::wstring& filePath);

        static FileType GetJsonFileType(const std::filesystem::path& path);

        // プロジェクトを保存
		static void SaveProject(EditorManager* editorManager, SceneManager* sceneManager, GameWorld* gameWorld, ECSManager* ecs, EngineCommand* engineCommand);
        // プロジェクトフォルダを読み込む
        static bool LoadProjectFolder(const std::wstring& projectFolderPath, EngineCommand* engineCommand);
        /// CSVを2次元vectorに読み込む関数
        // csv読み込み
        static std::vector<std::vector<std::string>> LoadCSV(const std::string& filePath);
        /// CSVを2次元vector<int>に読み込む関数
        static std::vector<std::vector<int>> LoadCSV_Int(const std::string& filePath);

        static FileType DetectFileType(const nlohmann::json& j)
        {
            std::string type = j.value("fileType", "");
            if (type == "ChoProject") return FileType::ChoProject;
            if (type == "EngineConfig") return FileType::EngineConfig;
            if (type == "GameSettings") return FileType::GameSettings;
            if (type == "SceneFile") return FileType::SceneFile;
			if (type == "ModelFile") return FileType::ModelFile;
			if (type == "ImageFile") return FileType::ImageFile;
			if (type == "SoundFile") return FileType::SoundFile;
			if (type == "EffectFile") return FileType::EffectFile;
			if (type == "ScriptFile") return FileType::ScriptFile;
			if (type == "GameParameter") return FileType::GameParameter;
			if (type == "PrefabFile") return FileType::PrefabFile;
            return FileType::Unknown;
        }
        // GUID 生成
        static std::string GenerateGUID();
        static std::wstring m_sProjectName;
        static std::wstring m_sProjectFolderPath;
        static inline FolderNode g_ProjectFiles;
		static inline GameSettingsInfo g_GameSettings;
        static inline CacheFile g_CacheFile;

        class ScriptProject
        {
        public:
            struct StagedFiles
            {
                path originalDll;
                path originalPdb;   // 無い場合は空
                path stageDir;      // 例: C:\path\to\bin\.stage\1234-20251010-120102-123456
                path stagedDll;
                path stagedPdb;     // 無い場合は空
            };

            static void GenerateSolutionAndProject();
			static void UpdateVcxproj();
            static void UpdateFilters(const std::string& filterPath);
            static void GenerateScriptFiles(const std::string& scriptName);
            static void LoadProjectPath(const std::wstring& projectName);
            static void LoadScriptDLL();
			static void UnloadScriptDLL();
            static bool BuildScriptDLL();
			static std::vector<std::string> GetScriptFiles();
			static HMODULE GetScriptDLLHandle() { return m_DllHandle; }
			static bool LoadPDB(const path& dllPath);
            static void UnloadPDB();
            static bool WaitForBuild(const std::wstring& dllPath, int timeoutMs = 60000);
            static bool WaitForBuildNotification(int timeoutMs);
            static bool ConnectPipeToBuildWatcher(const int& timeoutMs);
            // メッセージ送信
            static void SendMessageToBuildWatcher(const std::wstring& msg);
            // メッセージ受信
            static std::wstring WaitForAckFromBuildWatcher(DWORD timeoutMs);
            static bool TestPipeMessage();

            static bool SaveAndBuildSolution(const bool& isBuild = true, const bool& isDebugger = false);
            // ファイル追加
            static bool AddScriptFileToProject(const std::wstring& scriptName);
            static bool AddClassFileToProject(const std::wstring& className);

            static std::wstring norm_path(const std::wstring& p);
            static bool iequals(const std::wstring& a, const std::wstring& b);

            static std::wstring NowStamp();

            static bool CopyWithRetry(const path& src, const path& dst,
                int retries = 10, DWORD waitMs = 30, bool overwrite = true);

            static StagedFiles StageDllAndPdbInSiblingFolder(const path& originalDll,
                const std::wstring& stageRootName = L".stage");

            static void CleanupStage(const StagedFiles& staged);

            static std::string m_SlnGUID;
			static std::string m_ProjGUID;
			static std::string m_SlnPath;
			static std::string m_ProjPath;
            static HMODULE m_DllHandle;
			static DWORD64 m_PDBBaseAddress;


            // Pipe
            static void ClosePipe();
			static HANDLE m_ReadPipe;// 読み取り用パイプ
			static HANDLE m_WritePipe;// 書き込み用パイプ
            static bool m_IsAttached;
        };

        class GameBuilder
        {
        public:
			static std::wstring SelectFolderDialog();
			static void CopyFilesToBuildFolder(EngineCommand* engineCommand, const std::wstring& folderPath);
            static std::wstring GetEnvVar(const wchar_t* name);

        };
    };
}

