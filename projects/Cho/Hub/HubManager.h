#pragma once
#include <filesystem>
class PlatformLayer;
class CoreSystem;
class EngineCommand;
class HubManager
{
public:
	// Constructor
	HubManager(
		PlatformLayer* platformLayer,
		CoreSystem* coreSystem,
		EngineCommand* engineCommand,
		bool isGameRuntime) :
		m_pPlatformLayer(platformLayer),
		m_pCoreSystem(coreSystem),
			m_pEngineCommand(engineCommand),
			m_IsGameRuntime(isGameRuntime)
	{
	}
	// Destructor
	~HubManager()
	{
	}
	// 初期化
	void Initialize();
	// 更新
	void Update();
	// ウィンドウ
	void Window();
	
	bool IsRun() const { return m_IsRun; }

    void RenderFullScreenUI();
	void ShowSidebar();

	void ShowMainContent();

	void GetCurrentBranch();

	bool CheckBranchChanged();

	void ReloadProject();

private:
	std::string ReadCurrentBranch()
	{
		std::ifstream headFile(m_GitHeadPath);
		if (!headFile.is_open())
		{
			return std::string();
		}

		std::string line;
		std::getline(headFile, line);
		const std::string prefix = "ref: refs/heads/";
		if (line.find(prefix) == 0)
		{
			return line.substr(prefix.length());
		}

		// detached HEAD などの場合は SHA をそのまま返す
		return line;
	}

	PlatformLayer* m_pPlatformLayer = nullptr;
	CoreSystem* m_pCoreSystem = nullptr;
	EngineCommand* m_pEngineCommand = nullptr;
	bool m_IsRun = true;

	// 現在のブランチ
	std::string m_LastBranch = "";
	std::filesystem::path m_GitHeadPath = "";

	bool m_IsGameRuntime = false;
};

