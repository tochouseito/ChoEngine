#pragma once
#include <unordered_map>
#include <string>

// 存在する要素にコピーする
void copyIfExists(const std::unordered_map<int, std::string>& source, std::unordered_map<int, std::string>& destination)
{
    for (const auto& [key, value] : source)
    {
        auto it = destination.find(key);
        if (it != destination.end())
        {
            it->second = value;
        }
    }
}
