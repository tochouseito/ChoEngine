#pragma once
#define NOMINMAX // Windowのminmaxマクロを除外
#include <xaudio2.h>
#include <fstream>
/*ファイル構造体*/
	/*チャンクヘッダ*/
struct ChunkHeader
{
	char id[4];/*チャンク毎のID*/
	int32_t size;/*チャンクのサイズ*/
};
/*RIFFヘッダチャンク*/
struct RiffHeader
{
	ChunkHeader chunk;/*”RIFF"*/
	char type[4];/*"WAVE"*/
};
/*FMTチャンク*/
struct FormatChunk
{
	ChunkHeader chunk;/*"fmt"*/
	WAVEFORMATEX fmt;/*波形フォーマット*/
};
/*音声データ*/
struct SoundData
{
	std::string name = {}; /* 音声データの名前（ファイル名など） */
	WAVEFORMATEX wfex = {};     /*波形フォーマット*/
	BYTE* pBuffer = nullptr;         /*バッファの先頭アドレス*/
	unsigned int bufferSize = 0;/*バッファのサイズ*/
	IXAudio2SourceVoice* pSourceVoice = nullptr; /* SourceVoice を保持 */
	float currentVolume = 1.0f;    /* 現在のボリューム */
	bool isPlaying = false; /* 再生中かどうか */
};