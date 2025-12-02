#include "pch.h"
#include "MaterialEditor.h"
#include <imgui-node-editor/imgui_node_editor.h>
#include "Editor/EditorManager/EditorManager.h"
#include "Resources/ResourceManager/ResourceManager.h"
#include <map>

namespace ed = ax::NodeEditor;

struct MaterialEditor::Impl
{
    struct LinkInfo
    {
        ed::LinkId Id;
        ed::PinId  InputId;
        ed::PinId  OutputId;
    };
	ed::EditorContext* m_NodeEditorContext = nullptr; // Node Editorのコンテキスト
	ImVector<LinkInfo> m_LinkInfos; // リンク情報のリスト

    enum class PinType
    {
        Flow,
        Bool,
        Int,
        Float,
        String,
        Object,
        Function,
        Delegate,
    };

    enum class PinKind
    {
        Output,
        Input
    };

    enum class NodeType
    {
        Blueprint,
        Simple,
        Tree,
        Comment,
        Houdini
    };

    struct Node;

    struct Pin
    {
        ed::PinId   ID;
        Node* Node;
        std::string Name;
        PinType     Type;
        PinKind     Kind;

        Pin(int id, const char* name, PinType type) :
            ID(id), Node(nullptr), Name(name), Type(type), Kind(PinKind::Input)
        {
        }
    };

    struct Node
    {
        ed::NodeId ID;
        std::string Name;
        std::vector<Pin> Inputs;
        std::vector<Pin> Outputs;
        ImColor Color;
        NodeType Type;
        ImVec2 Size;

        std::string State;
        std::string SavedState;

        Node(int id, const char* name, ImColor color = ImColor(255, 255, 255)) :
            ID(id), Name(name), Color(color), Type(NodeType::Blueprint), Size(0, 0)
        {
        }
    };

    struct Link
    {
        ed::LinkId ID;

        ed::PinId StartPinID;
        ed::PinId EndPinID;

        ImColor Color;

        Link(ed::LinkId id, ed::PinId startPinId, ed::PinId endPinId) :
            ID(id), StartPinID(startPinId), EndPinID(endPinId), Color(255, 255, 255)
        {
        }
    };

    struct NodeIdLess
    {
        bool operator()(const ed::NodeId& lhs, const ed::NodeId& rhs) const
        {
            return lhs.AsPointer() < rhs.AsPointer();
        }
    };

	std::vector<Node> m_Nodes; // ノードのリスト
    std::vector<Link> m_Links; // リンクのリスト
	ImTextureID m_HeaderBackground; // ヘッダーの背景テクスチャ
	ImTextureID m_SaveIcon; // 保存アイコンのテクスチャ
	ImTextureID m_restoreIcon; // リストアアイコンのテクスチャ
	std::map<ed::NodeId, float, NodeIdLess> m_NodeTouchTime; // ノードのタッチ時間を管理するマップ
};

MaterialEditor::MaterialEditor(EditorManager* editorManager) :
	BaseEditor(editorManager), impl(std::make_unique<Impl>())
{
	//// node editorの初期化
	//ed::Config config;
	//config.SettingsFile = "MaterialEditor.json"; // 設定ファイルのパス
	//impl->m_NodeEditorContext = ed::CreateEditor(&config);
	//// ed::SetCurrentEditor(m_NodeEditorContext);
	//ResourceManager* resM = m_EditorManager->GetEngineCommand()->GetResourceManager();
	//TextureManager* texM = resM->GetTextureManager();
 //   std::string dataPaht = "External/imgui_Extensions/imgui-node-editor/examples/blueprints-example/data/";
 //   texM->LoadTextureFile(
 //       dataPaht + "BlueprintBackground.png");
 //   texM->LoadTextureFile(
 //       dataPaht + "ic_save_white_24dp.png");
 //   texM->LoadTextureFile(
 //       dataPaht + "ic_restore_white_24dp.png");

 //   // handle
	//PixelBuffer* headerBackground = resM->GetBuffer<PixelBuffer>(texM->GetTextureID(L"header_background.png"));
 //   PixelBuffer* saveIcon = resM->GetBuffer<PixelBuffer>(texM->GetTextureID(L"ic_save_white_24dp.png"));
 //   PixelBuffer* restoreIcon = resM->GetBuffer<PixelBuffer>(texM->GetTextureID(L"ic_restore_white_24dp.png"));
 //   impl->m_HeaderBackground = (ImTextureID)headerBackground->GetSRVGpuHandle().ptr;
	//impl->m_SaveIcon = (ImTextureID)saveIcon->GetSRVGpuHandle().ptr;
	//impl->m_restoreIcon = (ImTextureID)restoreIcon->GetSRVGpuHandle().ptr;
}

MaterialEditor::~MaterialEditor()
{
	if (impl->m_NodeEditorContext)
	{
		ed::DestroyEditor(impl->m_NodeEditorContext);
        impl->m_NodeEditorContext = nullptr;
	}
	// ed::SetCurrentEditor(nullptr);
}

void MaterialEditor::Initialize()
{
	
}

void MaterialEditor::Update()
{
	//Window();
}

void MaterialEditor::Window()
{
	
}

void MaterialEditor::ImGuiEx_BeginColumn()
{
    ImGui::BeginGroup();
}

void MaterialEditor::ImGuiEx_NextColumn()
{
    ImGui::EndGroup();
    ImGui::SameLine();
    ImGui::BeginGroup();
}

void MaterialEditor::ImGuiEx_EndColumn()
{
    ImGui::EndGroup();
}

void MaterialEditor::BasicNodeExample()
{
    ed::SetCurrentEditor(impl->m_NodeEditorContext);
    auto& io = ImGui::GetIO();

    // ウィンドウのフラグ
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking;

    ImGui::Begin("Node Editor", nullptr, windowFlags); // ← ウィンドウ名を明示
    ImGui::Text("FPS: %.2f (%.2gms)", io.Framerate, io.Framerate ? 1000.0f / io.Framerate : 0.0f);
    ImGui::Separator();

    ImVec2 size = ImGui::GetContentRegionAvail(); // ← ウィンドウにフィット
    // Start interaction with editor.
    ed::Begin("My Editor", size);

    int uniqueId = 1;

    //
    // 1) Commit known data to editor
    //

    // Submit Node A
    ed::NodeId nodeA_Id = uniqueId++;
    ed::PinId  nodeA_InputPinId = uniqueId++;
    ed::PinId  nodeA_OutputPinId = uniqueId++;

    if (m_FirstFrame)
    {
        ed::SetNodePosition(nodeA_Id, ImVec2(10, 10));
    }
    ed::BeginNode(nodeA_Id);
    ImGui::Text("Node A");
    ed::BeginPin(nodeA_InputPinId, ed::PinKind::Input);
    ImGui::Text("-> In");
    ed::EndPin();
    ImGui::SameLine();
    ed::BeginPin(nodeA_OutputPinId, ed::PinKind::Output);
    ImGui::Text("Out ->");
    ed::EndPin();
    ed::EndNode();

    // Submit Node B
    ed::NodeId nodeB_Id = uniqueId++;
    ed::PinId  nodeB_InputPinId1 = uniqueId++;
    ed::PinId  nodeB_InputPinId2 = uniqueId++;
    ed::PinId  nodeB_OutputPinId = uniqueId++;

    if (m_FirstFrame)
    {
        ed::SetNodePosition(nodeB_Id, ImVec2(210, 60));
    }
    ed::BeginNode(nodeB_Id);
    ImGui::Text("Node B");
    ImGuiEx_BeginColumn();
    ed::BeginPin(nodeB_InputPinId1, ed::PinKind::Input);
    ImGui::Text("-> In1");
    ed::EndPin();
    ed::BeginPin(nodeB_InputPinId2, ed::PinKind::Input);
    ImGui::Text("-> In2");
    ed::EndPin();
    ImGuiEx_NextColumn();
    ed::BeginPin(nodeB_OutputPinId, ed::PinKind::Output);
    ImGui::Text("Out ->");
    ed::EndPin();
    ImGuiEx_EndColumn();
    ed::EndNode();

    // Submit Links
    for (auto& linkInfo : impl->m_LinkInfos)
        ed::Link(linkInfo.Id, linkInfo.InputId, linkInfo.OutputId);

    //
    // 2) Handle interactions
    //

    // Handle creation action, returns true if editor want to create new object (node or link)
    if (ed::BeginCreate())
    {
        ed::PinId inputPinId, outputPinId;
        if (ed::QueryNewLink(&inputPinId, &outputPinId))
        {
            // QueryNewLink returns true if editor want to create new link between pins.
            //
            // Link can be created only for two valid pins, it is up to you to
            // validate if connection make sense. Editor is happy to make any.
            //
            // Link always goes from input to output. User may choose to drag
            // link from output pin or input pin. This determine which pin ids
            // are valid and which are not:
            //   * input valid, output invalid - user started to drag new ling from input pin
            //   * input invalid, output valid - user started to drag new ling from output pin
            //   * input valid, output valid   - user dragged link over other pin, can be validated

            if (inputPinId && outputPinId) // both are valid, let's accept link
            {
                // ed::AcceptNewItem() return true when user release mouse button.
                if (ed::AcceptNewItem())
                {
                    // Since we accepted new link, lets add one to our list of links.
                    impl->m_LinkInfos.push_back({ ed::LinkId(m_NextLinkId++), inputPinId, outputPinId });

                    // Draw new link.
                    ed::Link(impl->m_LinkInfos.back().Id, impl->m_LinkInfos.back().InputId, impl->m_LinkInfos.back().OutputId);
                }

                // You may choose to reject connection between these nodes
                // by calling ed::RejectNewItem(). This will allow editor to give
                // visual feedback by changing link thickness and color.
            }
        }
    }
    ed::EndCreate(); // Wraps up object creation action handling.


    // Handle deletion action
    if (ed::BeginDelete())
    {
        // There may be many links marked for deletion, let's loop over them.
        ed::LinkId deletedLinkId;
        while (ed::QueryDeletedLink(&deletedLinkId))
        {
            // If you agree that link can be deleted, accept deletion.
            if (ed::AcceptDeletedItem())
            {
                // Then remove link from your data.
                for (auto& link : impl->m_LinkInfos)
                {
                    if (link.Id == deletedLinkId)
                    {
                        impl->m_LinkInfos.erase(&link);
                        break;
                    }
                }
            }

            // You may reject link deletion by calling:
            // ed::RejectDeletedItem();
        }
    }
    ed::EndDelete(); // Wrap up deletion action



    // End of interaction with editor.
    ed::End();
    //ed::Begin("MyCanvas", size);

    //int uniqueId = 1;

    //ed::BeginNode(uniqueId++);
    //ImGui::PushID(uniqueId); // ノードごとにスコープを分ける
    //ImGui::Text("Node A###node1");

    //ed::BeginPin(uniqueId++, ed::PinKind::Input);
    //ImGui::Text("-> In###pin1");
    //ed::EndPin();

    //ImGui::SameLine();

    //ed::BeginPin(uniqueId++, ed::PinKind::Output);
    //ImGui::Text("Out ->###pin2");
    //ed::EndPin();

    //ImGui::PopID();
    //ed::EndNode();

    //ed::End();  // NodeEditor
    ImGui::End(); // ウィンドウ終了
    if (m_FirstFrame)
    {
        ed::NavigateToContent(0.0f);

        m_FirstFrame = false;
    }
    ed::SetCurrentEditor(nullptr);
}
