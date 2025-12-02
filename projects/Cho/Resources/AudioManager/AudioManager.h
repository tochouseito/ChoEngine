#pragma once
#include "Core/Utility/SoundData.h"
#include <wrl.h>
#include "Core/Utility/FVector.h"
#include <unordered_map>

class ResourceManager;
class AudioManager
{
public:
	AudioManager(ResourceManager* resourceManager) :
		m_pResourceManager(resourceManager)
	{
		Initialize();
	}
	~AudioManager() 
	{
		Finalize();
	};
	void Initialize();

	bool SoundLordWave(const std::filesystem::path& filePath);

	/*音声データ解放*/
	void SoundUnLord(const uint32_t& index);

	/*音声再生*/
	void SoundPlayWave(const uint32_t& index, bool loop = false); // ループフラグを追加
	void SoundStop(const uint32_t& index); // Stop関数を追加
	void SoundStopFadeOut(const uint32_t& index, float duration);
	void SoundPlayWave(SoundData& soundData, bool loop = false); // ループフラグを追加
	void SoundStop(SoundData& soundData); // Stop関数を追加
	/// <summary>
	/// 終了処理
	/// </summary>
	void Finalize();

	IXAudio2* GetXAudio2() { return m_XAudio2.Get(); }

	/// <summary>
	/// ボリュームを変更する
	/// </summary>
	/// <param name="soundData">音声データ</param>
	/// <param name="volume">設定するボリューム（0.0～1.0）</param>
	void SetVolume(SoundData& soundData, float volume);

	// コンテナ取得
	const SoundData& GetSoundData(const uint32_t& index) const
	{
		return m_SoundData[index];
	}

	const SoundData& GetSoundData(const std::string& name) const
	{
		auto it = m_SoundDataToName.find(name);
		if (it != m_SoundDataToName.end())
		{
			return m_SoundData[it->second];
		}
		//throw std::runtime_error("Sound data not found: " + name);
	}

	SoundData CreateSoundData(const std::string& name)
	{
		SoundData result;
		if (m_SoundDataToName.contains(name))
		{
			SoundData& existingData = m_SoundData[m_SoundDataToName[name]];
			result.name = existingData.name;
			result.wfex = existingData.wfex;
			result.pBuffer = existingData.pBuffer;
			result.bufferSize = existingData.bufferSize;
			result.isPlaying = false;
			result.pSourceVoice = nullptr;
			m_XAudio2->CreateSourceVoice(&result.pSourceVoice, &result.wfex);
		}
		else
		{
			result.name = "";
		}
		return result;
	}

	// 名前で検索してインデックスを取得する
	std::optional<uint32_t> GetSoundDataIndex(const std::string& name)
	{
		auto it = m_SoundDataToName.find(name);
		if (it != m_SoundDataToName.end())
		{
			return it->second;
		}
		return std::nullopt;
	}
	// 名前コンテナ取得
	const std::unordered_map<std::string, uint32_t>& GetSoundDataToName() const
	{
		return m_SoundDataToName;
	}
private:
	// ResourceManagerのポインタ
	ResourceManager* m_pResourceManager = nullptr;
	Microsoft::WRL::ComPtr<IXAudio2> m_XAudio2;
	IXAudio2MasteringVoice* m_MasterVoice = nullptr;
	// サウンドコンテナ
	FVector<SoundData> m_SoundData;
	// 名前検索用コンテナ
	std::unordered_map<std::string, uint32_t> m_SoundDataToName;
};

