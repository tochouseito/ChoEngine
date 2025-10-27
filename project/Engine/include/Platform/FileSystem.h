#pragma once
// === C++ Standard Library ===
#include <string>
#include <string_view>
#include <fstream>

namespace Theatria::Platform::FileSystem
{
    /// @brief 指定されたパスが存在するかどうかを判定します。
    /// @param path 確認するファイルまたはディレクトリへのパス。
    /// @return パスが存在する場合はtrue、存在しない場合はfalseを返します。
    bool exists(std::string_view path);
    bool exists(std::wstring_view path);

    bool create_directory(std::string_view path);
    bool create_directory(std::wstring_view path);
};

