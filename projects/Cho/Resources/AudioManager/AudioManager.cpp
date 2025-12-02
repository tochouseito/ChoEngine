#include "pch.h"
#include "AudioManager.h"
#include <thread>
#include <chrono>
#include <filesystem>

void AudioManager::Initialize()
{
	HRESULT result;
	/*xAudioエンジンのインスタンスを生成*/
	result = XAudio2Create(&m_XAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	assert(SUCCEEDED(result));
	/*マスターボイスを生成*/
	result = m_XAudio2->CreateMasteringVoice(&m_MasterVoice);
	assert(SUCCEEDED(result));
}

bool AudioManager::SoundLordWave(const std::filesystem::path& filePath)
{
	/*ファイルオープン*/
	/*ファイル入力ストリームのインスタンス*/
	std::ifstream file;
	/*.wavファイルをバイナリモードで開く*/
	file.open(filePath.c_str(), std::ios_base::binary);
	/*ファイルオープン失敗を検出する*/
	assert(file.is_open());

	/*.wavデータ読み込み*/
	/*RIFFヘッダーの読み込み*/
	RiffHeader riff;
	file.read((char*)&riff, sizeof(riff));
	/*ファイルがRIFFかチェック*/
	if (strncmp(riff.chunk.id, "RIFF", 4) != 0)
	{
		assert(0);
	}
	/*タイプがWAVEかチェック*/
	if (strncmp(riff.type, "WAVE", 4) != 0)
	{
		assert(0);
	}
	///*Formatチャンクの読み込み*/
	//FormatChunk format = {};
	///*チャンクヘッダーの確認*/
	//// fmt チャンクを探す
	//while (true) {
	//	file.read(reinterpret_cast<char*>(&format.chunk), sizeof(ChunkHeader));
	//	if (file.eof()) {
	//		assert(0); // チャンクが見つからなかった場合
	//	}
	//	if (strncmp(format.chunk.id, "fmt ", 4) == 0) {
	//		break; // fmt チャンクを見つけた
	//	}
	//	// 他のチャンクの場合、サイズ分だけ読み飛ばす
	//	file.seekg(format.chunk.size, std::ios_base::cur);
	//}
	///*チャンク本体の読み込み*/
	//assert(format.chunk.size <= sizeof(format.fmt));
	//file.read((char*)&format.fmt, format.chunk.size);

	/*Formatチャンクの読み込み*/
	FormatChunk format = {};
	/*チャンクヘッダーの確認*/
	while (true)
	{
		file.read(reinterpret_cast<char*>(&format.chunk), sizeof(ChunkHeader));
		if (file.eof())
		{
			assert(0); // チャンクが見つからなかった場合
		}
		if (strncmp(format.chunk.id, "fmt ", 4) == 0)
		{
			break; // fmt チャンクを見つけた
		}
		// 他のチャンクの場合、サイズ分だけ読み飛ばす
		file.seekg(format.chunk.size, std::ios_base::cur);
	}

	// メモリを確保して format.fmt に読み込む
	std::vector<BYTE> fmtBuffer(format.chunk.size);
	file.read(reinterpret_cast<char*>(fmtBuffer.data()), format.chunk.size);

	// `format.fmt` にコピーする（サイズが大きい場合を考慮）
	memcpy(&format.fmt, fmtBuffer.data(), std::min(static_cast<int32_t>(sizeof(WAVEFORMATEX)), format.chunk.size));

	// `WAVEFORMATEXTENSIBLE` の場合の対応
	if (format.fmt.wFormatTag == WAVE_FORMAT_EXTENSIBLE && format.chunk.size >= sizeof(WAVEFORMATEXTENSIBLE))
	{
		WAVEFORMATEXTENSIBLE* wfex = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(fmtBuffer.data());
		format.fmt = wfex->Format; // 基本フォーマットをコピー
	}

	/*Dataチャンクの読み込み*/
	ChunkHeader data;
	/*dataを読み込み、data チャンクを見つけるまでループする*/
	while (true)
	{
		file.read(reinterpret_cast<char*>(&data), sizeof(data));
		if (file.eof())
		{
			//throw std::runtime_error("No data chunk found in WAV file.");
			assert(0);
		}
		if (strncmp(data.id, "data", 4) == 0)
		{
			break;
		}
		/*目的のチャンク (data チャンク) を見つけるまで、サイズ分だけ読み飛ばす*/
		file.seekg(data.size, std::ios_base::cur);
	}
	/*Dataチャンクのデータ部（波形データ）の読み込み*/
	char* pBuffer = new char[data.size];
	file.read(pBuffer, data.size);

	/*ファイルクローズ*/
	/*Waveファイルを閉じる*/
	file.close();
	/*読み込んだ音声データをreturn*/
	/*returnするための音声データ*/
	SoundData soundData = {};

	soundData.wfex = format.fmt;
	soundData.pBuffer = reinterpret_cast<BYTE*>(pBuffer);
	soundData.bufferSize = data.size;
	std::string name = filePath.stem().string();
	soundData.name = name;
	// 音声データをコンテナに追加
	uint32_t index = static_cast<uint32_t>(m_SoundData.push_back(std::move(soundData)));
	// 名前をキーにして音声データのインデックスを保存
	m_SoundDataToName[name] = index;
	return true;
}

void AudioManager::SoundUnLord(const uint32_t& index)
{
	SoundData* soundData = &m_SoundData[index];
	if (!soundData||!soundData->pSourceVoice)
	{
		return; // nullptr の場合は何もしない
	}

	/*再生中なら停止させる*/
	if (soundData->isPlaying)
	{
		SoundStop(index);
	}

	/*バッファのメモリを解放*/
	if (soundData->pBuffer)
	{
		delete[] soundData->pBuffer;
		soundData->pBuffer = nullptr;
	}

	soundData->bufferSize = 0;

	/*ソースボイスの解放*/
	if (soundData->pSourceVoice)
	{
		soundData->pSourceVoice->DestroyVoice();
		soundData->pSourceVoice = nullptr;
	}

	// `memset` を使って安全にクリア
	memset(&soundData->wfex, 0, sizeof(WAVEFORMATEX));
}

void AudioManager::SoundPlayWave(const uint32_t& index, bool loop)
{
	SoundData& soundData = m_SoundData[index];
	HRESULT result;
	if (!soundData.pSourceVoice)
	{
		result = m_XAudio2->CreateSourceVoice(&soundData.pSourceVoice, &soundData.wfex);
		assert(SUCCEEDED(result));
	}

	XAUDIO2_BUFFER buf{};
	buf.pAudioData = soundData.pBuffer;
	buf.AudioBytes = soundData.bufferSize;
	buf.Flags = XAUDIO2_END_OF_STREAM;

	if (loop)
	{
		buf.LoopBegin = 0;
		buf.LoopLength = 0; // 無限ループ設定
		buf.LoopCount = XAUDIO2_LOOP_INFINITE;
	}
	soundData.pSourceVoice->FlushSourceBuffers();
	result = soundData.pSourceVoice->SubmitSourceBuffer(&buf);
	assert(SUCCEEDED(result));

	result = soundData.pSourceVoice->Start();
	assert(SUCCEEDED(result));

	soundData.isPlaying = true;
}

void AudioManager::SoundStop(const uint32_t& index)
{
	SoundData& soundData = m_SoundData[index];
	if (soundData.pSourceVoice)
	{
		soundData.pSourceVoice->Stop(0);
		soundData.pSourceVoice->FlushSourceBuffers();
	}
	soundData.isPlaying = false;
}

void AudioManager::SoundStopFadeOut(const uint32_t& index, float duration)
{
	SoundData& soundData = m_SoundData[index];
	if (!soundData.pSourceVoice)
	{
		return;
	}

	// 現在のボリュームを取得
	float currentVolume = soundData.currentVolume;

	// フェードアウトのステップ設定
	const int steps = 50; // フェードアウトを50ステップに分割
	const float stepDuration = duration / steps; // 1ステップの時間
	const float volumeDecrement = currentVolume / steps; // 1ステップで減少するボリューム

	for (int i = 0; i < steps; ++i)
	{
		currentVolume -= volumeDecrement; // ボリュームを減らす
		if (currentVolume < 0.0f)
		{
			currentVolume = 0.0f;
		}

		// ボリュームを設定
		SetVolume(soundData, currentVolume);

		// スリープして次のステップに進む
		std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(stepDuration * 1000)));
	}

	// 音声を停止
	SoundStop(index);
}

void AudioManager::SoundPlayWave(SoundData& soundData, bool loop)
{
	HRESULT result;
	if (!soundData.pSourceVoice)
	{
		result = m_XAudio2->CreateSourceVoice(&soundData.pSourceVoice, &soundData.wfex);
		assert(SUCCEEDED(result));
	}

	XAUDIO2_BUFFER buf{};
	buf.pAudioData = soundData.pBuffer;
	buf.AudioBytes = soundData.bufferSize;
	buf.Flags = XAUDIO2_END_OF_STREAM;

	if (loop)
	{
		buf.LoopBegin = 0;
		buf.LoopLength = 0; // 無限ループ設定
		buf.LoopCount = XAUDIO2_LOOP_INFINITE;
	}
	soundData.pSourceVoice->FlushSourceBuffers();
	result = soundData.pSourceVoice->SubmitSourceBuffer(&buf);
	assert(SUCCEEDED(result));

	result = soundData.pSourceVoice->Start();
	assert(SUCCEEDED(result));

	soundData.isPlaying = true;
}

void AudioManager::SoundStop(SoundData& soundData)
{
	if (soundData.pSourceVoice)
	{
		soundData.pSourceVoice->Stop(0);
		soundData.pSourceVoice->FlushSourceBuffers();
	}
	soundData.isPlaying = false;
}

void AudioManager::Finalize()
{
	/*全ての音声データを解放*/
	for (uint32_t i = 0; i < m_SoundData.size(); ++i)
	{
		SoundUnLord(i);
	}
	/*XAudio2解放*/
	m_XAudio2.Reset();
}

void AudioManager::SetVolume(SoundData& soundData, float volume)
{
	HRESULT result;
	if (soundData.pSourceVoice)
	{
		// ボリュームを設定する
		result = soundData.pSourceVoice->SetVolume(volume);

		assert(SUCCEEDED(result)); // ボリューム設定が失敗した場合にアサート

		// 現在のボリューム値を保存
		soundData.currentVolume = volume;
	}
}
