#include "pch.h"
#include "TextureManager.h"
#include "Resources/ResourceManager/ResourceManager.h"
#include "Graphics/GraphicsEngine/GraphicsEngine.h"
#include "Core/ChoLog/ChoLog.h"
using namespace theatria;

void TextureManager::LoadTextureFile(const fs::path& filePath)
{
    // ファイル名部分のみ取得（ディレクトリパスを除去）
    std::wstring fileName = filePath.filename().wstring();
	std::wstring textureName = filePath.stem().wstring();

    // テクスチャファイルの形式をチェック（.dds, .png, .jpg など）
    if (fileName.ends_with(L".dds") || fileName.ends_with(L".png") || fileName.ends_with(L".jpg"))
    {
        // 読み込み済みテクスチャを検索
        if (m_TextureNameContainer.contains(textureName))
        {
            return; // 既に読み込み済みならスキップ
        }
        HRESULT hr;
        // テクスチャファイルを読んでプログラムで扱えるようにする
        DirectX::ScratchImage image{};
        std::wstring filePathW = filePath.wstring();

        if (filePathW.ends_with(L".dds"))
        {
            // .dds形式のファイルを読み込む
            hr = DirectX::LoadFromDDSFile(filePathW.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, image);
        } else
        {
            // 他のフォーマット (.png, .jpg など) を読み込む
            hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
        }
        Log::Write(LogLevel::Assert, "Texture loaded." + filePath.string(), hr);

        // ミップマップの生成
        DirectX::ScratchImage mipImages{};

        // 圧縮フォーマットかどうか調べる
        if (DirectX::IsCompressed(image.GetMetadata().format))
        {
            // 圧縮フォーマットならそのまま使うのでmoveする
            mipImages = std::move(image);
        } else
        {
            // テクスチャが小さい場合はミップマップ生成をスキップ
            if (image.GetMetadata().width > 1 && image.GetMetadata().height > 1)
            {
                hr = DirectX::GenerateMipMaps(
                    image.GetImages(),
                    image.GetImageCount(),
                    image.GetMetadata(),
                    DirectX::TEX_FILTER_SRGB,
                    0,
                    mipImages
                );
            } else
            {
                // 小さいテクスチャの場合はミップマップを生成せず、そのまま移動
                mipImages = std::move(image);
            }
        }
        Log::Write(LogLevel::Assert, "Mipmaps generated.", hr);
        // メタデータを取得
        const DirectX::TexMetadata& metadata = mipImages.GetMetadata();

        // TextureData作成
        TextureData texData;
        texData.name = textureName;// テクスチャ名
        texData.metadata = metadata;// メタデータ
        // テクスチャバッファの作成
        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Width = static_cast<UINT>(metadata.width);// Textureの幅
        resourceDesc.Height = static_cast<UINT>(metadata.height);// Textureの高さ
        resourceDesc.MipLevels = static_cast<UINT16>(metadata.mipLevels);// mipmapの数
        resourceDesc.DepthOrArraySize = static_cast<UINT16>(metadata.arraySize);// 奥行き or 配列Textureの配列数
        resourceDesc.Format = metadata.format;// Textureのフォーマット
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension);// Textureの次元
        resourceDesc.SampleDesc.Count = 1;// サンプル数
        texData.bufferIndex = m_ResourceManager->CreateTextureBuffer(resourceDesc, nullptr, D3D12_RESOURCE_STATE_COPY_DEST,texData.metadata.IsCubemap());
        // テクスチャデータをコピー
        UploadTextureDataEx(
            m_ResourceManager->GetBuffer<PixelBuffer>(texData.bufferIndex),
            mipImages
        );
        // テクスチャデータを追加
        m_TextureNameContainer[textureName] = static_cast<uint32_t>(m_Textures.push_back(std::move(texData)));
    }
}

void TextureManager::LoadEngineTexture()
{
    // テクスチャデータの読み込み
    std::string path = "Cho/Resources/EngineAssets/Texture";
    // ファイルパスのリストを取得
    for (const auto& entry : fs::directory_iterator(path))
    {
        // ファイルかどうかを確認し、DDS または WIC 画像フォーマットかどうかチェック
        if (fs::is_regular_file(entry.path()))
        {
			LoadTextureFile(entry.path());
        }
    }
}

void TextureManager::UploadTextureDataEx(PixelBuffer* pBuffer, const DirectX::ScratchImage& mipImages)
{
    CommandContext* context = m_GraphicsEngine->GetCommandContext();
    std::vector<D3D12_SUBRESOURCE_DATA> subresources;
    DirectX::PrepareUpload(m_Device, mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subresources);
    UINT64 intermediateSize = GetRequiredIntermediateSize(pBuffer->GetResource(), 0, static_cast<UINT>(subresources.size()));
    // Resourceの各種設定
    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;// UploadHeapを使う
    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Width = intermediateSize;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    std::unique_ptr<GpuResource> intermediateResource = std::make_unique<GpuResource>();
    intermediateResource->CreateResource(m_Device, heapProperties, D3D12_HEAP_FLAG_NONE, resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ);
    // メモリにデータをコピー
    m_GraphicsEngine->BeginCommandContext(context);
    UpdateSubresources(
        context->GetCommandList(),
        pBuffer->GetResource(),
        intermediateResource->GetResource(),
        0, 0,
        static_cast<UINT>(subresources.size()),
        subresources.data()
    );
    // Textureへの転送後は利用できるよう,
    // D3D12_RESOURCE_STATE_COPY_DESTから
    // D3D12_RESOURCE_STATE_GENERIC_READへ
    // ResourceStateを変更する
	context->CheckResourceStateTransition(pBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    m_GraphicsEngine->EndCommandContext(context, QueueType::Copy);
    m_GraphicsEngine->WaitForGPU(QueueType::Copy);
}

// ダミーテクスチャバッファを取得
PixelBuffer* TextureManager::GetDummyTextureBuffer()
{
    std::wstring fileName = L"Dummy202411181622";
    if (!m_TextureNameContainer.contains(fileName))
    {
		Log::Write(LogLevel::Assert, "DummyTexture is not found");
    }
    uint32_t index = m_TextureNameContainer[fileName];
    return m_ResourceManager->GetBuffer<PixelBuffer>(m_Textures[index].bufferIndex);
}
