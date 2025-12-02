#include "pch.h"
#include "include/Platform/FileSystem.h"
#include <filesystem>

/// @brief 指定されたパスが存在するかどうかを判定します。
/// @param path 確認するファイルまたはディレクトリへのパス。
/// @return パスが存在する場合はtrue、存在しない場合はfalseを返します。
bool Theatria::Platform::FileSystem::exists(std::string_view path)
{
    return std::filesystem::exists(std::filesystem::path(path.data()));
}

bool Theatria::Platform::FileSystem::exists(std::wstring_view path)
{
    return std::filesystem::exists(std::filesystem::path(path.data()));
}

bool Theatria::Platform::FileSystem::create_directory(std::string_view path)
{
    return std::filesystem::create_directory(std::filesystem::path(path.data()));
}

bool Theatria::Platform::FileSystem::create_directory(std::wstring_view path)
{
    return std::filesystem::create_directory(std::filesystem::path(path.data()));
}
