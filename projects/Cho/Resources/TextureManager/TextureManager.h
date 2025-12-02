#pragma once
#include <d3d12.h>
#include <DirectXTex.h>
#include <d3dx12.h>
#include "Core/Utility/FVector.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <filesystem>

namespace fs = std::filesystem;

struct TextureData
{
	std::wstring name;
	DirectX::TexMetadata metadata = {};
	uint32_t bufferIndex = UINT32_MAX;
};
class ResourceManager;
class GraphicsEngine;
class CommandContext;
class PixelBuffer;
class TextureManager
{
public:
	TextureManager(ResourceManager* resourceManager, GraphicsEngine* graphicsEngine, ID3D12Device8* device) :
		m_ResourceManager(resourceManager), m_GraphicsEngine(graphicsEngine), m_Device(device)
	{
		LoadEngineTexture();
	}
	~TextureManager()
	{

	}
	// テクスチャファイルの読み込み
	void LoadTextureFile(const fs::path& filePath);
	// Engineのリソースをロード
	void LoadEngineTexture();
	// Resourceのアップロード
	void UploadTextureDataEx(PixelBuffer* pBuffer, const DirectX::ScratchImage& mipImages);
	// ダミーテクスチャバッファを取得
	PixelBuffer* GetDummyTextureBuffer();
	// テクスチャが存在するか
	bool IsTextureExist(const std::wstring& name)
	{
		return m_TextureNameContainer.contains(name);
	}
	// IDを取得
	uint32_t GetTextureID(const std::wstring& name)
	{
		if (m_TextureNameContainer.contains(name))
		{
			return m_TextureNameContainer[name];
		}
		return 0;
	}
	// テクスチャデータを取得
	TextureData* GetTextureData(const uint32_t& id)
	{
		if (m_Textures.isValid(id))
		{
			return &m_Textures[id];
		}
		return &m_Textures[0];
	}
	// 名前でテクスチャデータを取得
	TextureData* GetTextureData(const std::wstring& name)
	{
		if (m_TextureNameContainer.contains(name))
		{
			return &m_Textures[m_TextureNameContainer[name]];
		}
		return &m_Textures[0];
	}
	// 名前コンテナを取得
	std::unordered_map<std::wstring, uint32_t>& GetTextureNameContainer()
	{
		return m_TextureNameContainer;
	}
private:
	ResourceManager* m_ResourceManager = nullptr;
	GraphicsEngine* m_GraphicsEngine = nullptr;
	ID3D12Device8* m_Device = nullptr;

	// TextureDataContainer
	FVector<TextureData> m_Textures;
	// 検索用名前コンテナ
	std::unordered_map<std::wstring, uint32_t> m_TextureNameContainer;
};

