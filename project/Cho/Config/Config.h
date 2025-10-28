#pragma once
#include <string>
#include <Vector3.h>

namespace theatria
{
	/*
	* エンジン、プロジェクトの設定や構成を保持するクラス
	*/
	class Config
	{
	public:
		struct ConfigData
		{
			// プロジェクト設定
			std::string projectName = "MyProject";		// プロジェクト名
			std::string projectVersion = "0.1.0";		// プロジェクトバージョン
			std::wstring projectPath = L"";				// プロジェクトパス
			std::string startScene = "MainScene";		// 開始シーン
			float3 gravity = { 0.0f, -9.81f, 0.0f };	// 重力
			std::string skyTexture = "";				// スカイテクスチャ

			// エンジン設定
			std::string engineVersion = "0.1.0";		// エンジンバージョン
			std::string graphicsAPI = "DirectX12";		// グラフィックスAPI
			bool debugMode = false;						// デバッグモード
			std::wstring exeDirectory = L"";			// 現在のディレクトリ

			// グラフィックス設定
			uint32_t windowWidth = 1280;				// ウィンドウ幅
			uint32_t windowHeight = 720;				// ウィンドウ高さ
			bool fullscreen = false;					// フルスクリーンモード
			bool vSync = true;							// 垂直同期
		};
        Config() {}
		~Config() = default;
		// 設定データの取得
		std::string GetProjectName() const { return configData.projectName; }
		std::string GetProjectVersion() const { return configData.projectVersion; }
		std::wstring GetProjectPath() const { return configData.projectPath; }
		std::string GetStartScene() const { return configData.startScene; }
		float3 GetGravity() const { return configData.gravity; }
		std::string GetSkyTexture() const { return configData.skyTexture; }

		std::string GetEngineVersion() const { return configData.engineVersion; }
		std::string GetGraphicsAPI() const { return configData.graphicsAPI; }
		bool IsDebugMode() const { return configData.debugMode; }
		std::wstring GetExeDirectory() const { return configData.exeDirectory; }

		uint32_t GetWindowWidth() const { return configData.windowWidth; }
		uint32_t GetWindowHeight() const { return configData.windowHeight; }
		bool IsFullscreen() const { return configData.fullscreen; }
		bool IsVSync() const { return configData.vSync; }
		// 設定データの更新
		void SetProjectName(const std::string& name) { configData.projectName = name; }
		void SetProjectVersion(const std::string& version) { configData.projectVersion = version; }
		void SetProjectPath(const std::wstring& path) { configData.projectPath = path; }
		void SetStartScene(const std::string& scene) { configData.startScene = scene; }
		void SetGravity(const float3& g) { configData.gravity = g; }
		void SetSkyTexture(const std::string& texture) { configData.skyTexture = texture; }

		void SetEngineVersion(const std::string& version) { configData.engineVersion = version; }
		void SetGraphicsAPI(const std::string& api) { configData.graphicsAPI = api; }
		void SetDebugMode(bool debug) { configData.debugMode = debug; }
		void SetExeDirectory(const std::wstring& dir) { configData.exeDirectory = dir; }

		void SetWindowWidth(uint32_t width) { configData.windowWidth = width; }
		void SetWindowHeight(uint32_t height) { configData.windowHeight = height; }
		void SetFullscreen(bool fullscreen) { configData.fullscreen = fullscreen; }
		void SetVSync(bool vsync) { configData.vSync = vsync; }
	private:
		ConfigData configData;
	};
}

