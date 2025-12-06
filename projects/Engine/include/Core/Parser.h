#pragma once
#include <vector>
#include "include/Graphics/PipelineManager.h"

namespace Theatria
{
    namespace Core
    {
        namespace Parser
        {
            std::vector<Graphics::GraphicsPipelineSettings> LoadGraphicsPipelines_ini();
            std::vector<Graphics::ComputePipelineSettings> LoadComputePipelines_ini();
            std::vector<Graphics::MeshPipelineSettings> LoadMeshPipelines_ini();
        }
    }
}

