#include "pch.h"
#include "Parser.h"
// === Theatria Engine Includes ===
#include "config/engineConfig.h"
#include "include/Core/LogAssert.h"
#include "include/Graphics/PipelineManager.h"
// === C++ Standard Library ===
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
// === JSONライブラリ ===
#include <External/nlohmann/json.hpp>

using namespace Theatria;

std::vector<Graphics::GraphicsPipelineSettings> Theatria::Core::Parser::LoadGraphicsPipelines_ini()
{
    std::vector<Graphics::GraphicsPipelineSettings> pipelines;

    // 1) パイプライン構成ファイルの読み込み
    std::ifstream file(Config::FilePath::GraphicsPipelines_iniPath);
    if (!file.is_open())
    {
        // ファイルオープン失敗
        Core::LogAssert::LogRuntime(
            std::source_location::current(),
            Core::LogAssert::SinkKind::MBox,
            Core::LogAssert::LogLevel::Error,
            "Parser",
            "Failed to open pipeline configuration file: {}", Config::FilePath::GraphicsPipelines_iniPath);
        Core::LogAssert::Check(false, "Parser",
            "Pipeline configuration file could not be opened");
    }

    // 2) ファイル内容のパース
    nlohmann::ordered_json j;
    file >> j;

    if (j.contains("fileType") && j["fileType"] != "GraphicsPipeline")
    {
        // ファイルタイプ違い
        Core::LogAssert::LogRuntime(
            std::source_location::current(),
            Core::LogAssert::SinkKind::MBox,
            Core::LogAssert::LogLevel::Error,
            "Parser",
            "Invalid pipeline configuration file type");
        Core::LogAssert::Check(false, "Parser",
            "Pipeline configuration file type is invalid");
    }

    // 3) パイプライン情報の抽出
    if (j.contains("pipelines") == false || j["pipelines"].is_array() == false)
    {
        // パイプライン情報なし
        Core::LogAssert::LogRuntime(
            std::source_location::current(),
            Core::LogAssert::SinkKind::MBox,
            Core::LogAssert::LogLevel::Error,
            "Parser",
            "No pipeline information found in configuration file");
        Core::LogAssert::Check(false, "Parser",
            "No pipeline information found in configuration file");
    }
    for (const auto& pipeline_ini : j["pipelines"])
    {
        Graphics::GraphicsPipelineSettings pipeline;
        pipeline.name = pipeline_ini.value("name", "");
        pipeline.vs = pipeline_ini.value("vs", "");
        pipeline.ps = pipeline_ini.value("ps", "");
        pipeline.gs = pipeline_ini.value("gs", "");
        pipeline.hs = pipeline_ini.value("hs", "");
        pipeline.ds = pipeline_ini.value("ds", "");
        pipelines.push_back(pipeline);
    }

    return pipelines;
}

std::vector<Graphics::ComputePipelineSettings> Theatria::Core::Parser::LoadComputePipelines_ini()
{
    return std::vector<Graphics::ComputePipelineSettings>();
}

std::vector<Graphics::MeshPipelineSettings> Theatria::Core::Parser::LoadMeshPipelines_ini()
{
    return std::vector<Graphics::MeshPipelineSettings>();
}
