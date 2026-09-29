#include "MingEngine/Engine/Render/RenderServer.hpp"

#include "MingEngine/Core/Memory.hpp"
#include "MingEngine/Core/Render/Vertex.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/BuiltinShaders.hpp"
#include "MingEngine/Engine/Render/IndexBuffer.hpp"
#include "MingEngine/Engine/Render/Renderer.hpp"
#include "MingEngine/Engine/Render/VertexBuffer.hpp"

#include <algorithm>

RenderServer::RenderServer(RendererServerConfig config) { m_renderer = MemNew<Renderer>(config); }

RenderServer::~RenderServer()
{
	MemDelete(m_renderer);
	m_renderer = nullptr;
}

void RenderServer::Startup()
{
	if (m_started)
	{
		return;
	}

	m_renderer->Startup();

	// Engine-facing default resources live here, so other systems never reach into the Renderer.
	// e.g. a mesh without a material, or a request without a shader, falls back to one of these
	m_defaultUnlit    = m_renderer->GetBuiltinShaderResource("DefaultUnlit", BuiltinShaders::DefaultUnlit);
	m_defaultLit      = m_renderer->GetBuiltinShaderResource("DefaultLit", BuiltinShaders::DefaultLit);
	m_defaultMaterial = CreateRef<MaterialResource>();

	// Empty texture slots keep the Renderer default textures bound, so a plain mesh renders white.
	// e.g. the default material only carries the lit shader and leaves every texture slot empty
	m_defaultMaterial->m_shaderResource = m_defaultLit;

	RegisterEvent("WindowResized", RenderServer::OnWindowResized);

	m_started = true;
}

void RenderServer::Shutdown()
{
	if (!m_started)
	{
		return;
	}

	UnregisterEvent("WindowResized", RenderServer::OnWindowResized);

	// 1) Detach light bases before releasing registered lights.
	for (RID lightRID : m_lightOwner.GetRIDList())
	{
		LightFree(lightRID);
	}

	// 2) Release every registered GPU resource while the Renderer is still alive.
	//    Releasing them later would be reported as a live D3D object leak.
	for (RID meshRID : m_meshOwner.GetRIDList())
	{
		MeshFree(meshRID);
	}

	for (RID textureRID : m_textureOwner.GetRIDList())
	{
		TextureFree(textureRID);
	}

	for (RID vertexBufferRID : m_vertexBufferOwner.GetRIDList())
	{
		FreeVertexBuffer(vertexBufferRID);
	}

	for (RID indexBufferRID : m_indexBufferOwner.GetRIDList())
	{
		FreeIndexBuffer(indexBufferRID);
	}

	for (RID canvasItemRID : m_canvasItemOwner.GetRIDList())
	{
		CanvasItemFree(canvasItemRID);
	}

	// 3) The defaults own builtin shader resources, so they are released before the Renderer shuts down.
	m_defaultMaterial = nullptr;
	m_defaultLit      = nullptr;
	m_defaultUnlit    = nullptr;

	m_renderer->Shutdown();
	m_started = false;
}

void RenderServer::BeginFrame() { m_renderer->BeginFrame(); }

void RenderServer::EndFrame() { m_renderer->EndFrame(); }

RID RenderServer::ViewportCreate() { return m_viewportOwner.CreateRID(); }

void RenderServer::ViewportFree(RID viewport)
{
	ViewportData* viewportData = m_viewportOwner.GetOrNull(viewport);
	if (viewportData == nullptr)
	{
		return;
	}

	for (RID itemRID : m_canvasItemOwner.GetRIDList())
	{
		CanvasItemData* item = m_canvasItemOwner.GetOrNull(itemRID);
		if (item != nullptr && item->m_viewport == viewport)
		{
			CanvasItemSetViewport(itemRID, RID::Invalid);
		}
	}
	for (RID layerRID : viewportData->m_canvasLayers)
	{
		CanvasLayerData* layer = m_canvasLayerOwner.GetOrNull(layerRID);
		if (layer != nullptr)
		{
			layer->m_viewport = RID::Invalid;
		}
	}

	// 1) GPU resources can only be released while the Renderer is alive.
	// 2) Drop the RID from the owner before its storage is freed.
	if (m_started)
	{
		m_renderer->DestroyViewportResources(viewportData);
	}

	m_viewportOwner.Free(viewport);
}

void RenderServer::ViewportSetActive(RID viewport, bool active)
{
	ViewportData* viewportData = m_viewportOwner.GetOrNull(viewport);
	if (viewportData != nullptr)
	{
		viewportData->m_active = active;
	}
}

void RenderServer::ViewportSetResolution(RID viewport, IntVec2 size)
{
	ViewportData* viewportData = m_viewportOwner.GetOrNull(viewport);
	if (viewportData == nullptr || size.x <= 0 || size.y <= 0 || viewportData->m_outputResolution == size)
	{
		return;
	}

	// Resize rebuilds every render target owned by this Viewport.
	m_renderer->ResizeViewport(viewportData, size);
}

void RenderServer::ViewportBeginFrame(RID viewport)
{
	ViewportData* viewportData = m_viewportOwner.GetOrNull(viewport);
	if (viewportData == nullptr)
	{
		return;
	}

	// 1) Requests only describe the current frame.
	for (auto& requests : viewportData->m_renderRequests)
	{
		requests.clear();
	}

	// 2) Lights are collected again by PrepareViewportData(), so drop the previous frame.
	viewportData->m_directionalLight = {};
	viewportData->m_pointLights.clear();
}

void RenderServer::ViewportSetCamera(RID viewport, RID camera)
{
	ViewportData* viewportData = m_viewportOwner.GetOrNull(viewport);
	if (viewportData != nullptr)
	{
		viewportData->m_camera = camera;
	}
}

void RenderServer::ViewportFreeCamera(RID viewport, RID camera)
{
	ViewportData* viewportData = m_viewportOwner.GetOrNull(viewport);

	// Only the camera that is currently bound clears itself, so another camera's
	// binding survives when a different camera leaves the tree.
	if (viewportData != nullptr && viewportData->m_camera == camera)
	{
		viewportData->m_camera = RID::Invalid;
	}
}

void RenderServer::ViewportSubmitRenderRequest(RID viewport, RenderRequest const& request)
{
	ViewportData* viewportData = m_viewportOwner.GetOrNull(viewport);
	if (viewportData == nullptr)
	{
		return;
	}

	// Fill the default shader for requests that carry none, because the backend rejects a null one.
	// e.g. a caller that only sets vertices, pass and blend state still gets a drawable request
	RenderRequest preparedRequest = request;
	if (preparedRequest.m_shader == nullptr)
	{
		preparedRequest.m_shader = m_defaultUnlit->GetShader();
	}

	viewportData->m_renderRequests[static_cast<size_t>(preparedRequest.m_pass)].push_back(preparedRequest);
}

GPUTexture* RenderServer::ViewportGetTexture(RID viewport) const
{
	ViewportData const* viewportData = m_viewportOwner.GetOrNull(viewport);
	return viewportData != nullptr ? viewportData->m_viewportOutputTexture : nullptr;
}

RID RenderServer::LightCreate(LightType type)
{
	LightData light;
	light.m_type = type;
	return m_lightOwner.CreateRID(light);
}

void RenderServer::LightFree(RID rid)
{
	if (m_lightOwner.GetOrNull(rid) == nullptr)
	{
		return;
	}

	// 1) Detach every instance before releasing its light data.
	for (RID instanceRID : m_instanceOwner.GetRIDList())
	{
		Instance* instance = m_instanceOwner.GetOrNull(instanceRID);
		if (instance->m_baseType == InstanceBaseType::Light && instance->m_base == rid)
		{
			InstanceSetBase(instanceRID, RID::Invalid);
		}
	}

	// 2) Release the light independently of its instances.
	m_lightOwner.Free(rid);
}

void RenderServer::LightSetColor(RID rid, Color const& color)
{
	LightData* light = m_lightOwner.GetOrNull(rid);
	if (light != nullptr)
	{
		light->m_color = color;
	}
}

void RenderServer::LightSetIntensity(RID rid, float intensity)
{
	LightData* light = m_lightOwner.GetOrNull(rid);
	if (light != nullptr)
	{
		light->m_intensity = intensity;
	}
}

void RenderServer::LightSetRange(RID rid, float range)
{
	LightData* light = m_lightOwner.GetOrNull(rid);
	if (light != nullptr)
	{
		light->m_range = range;
	}
}

void RenderServer::LightSetAttenuation(RID rid, float attenuation)
{
	LightData* light = m_lightOwner.GetOrNull(rid);
	if (light != nullptr)
	{
		light->m_attenuation = attenuation;
	}
}

void RenderServer::LightSetSpotAngle(RID rid, float angle)
{
	LightData* light = m_lightOwner.GetOrNull(rid);
	if (light != nullptr)
	{
		light->m_spotAngle = angle;
	}
}

void RenderServer::LightSetSpotAttenuation(RID rid, float attenuation)
{
	LightData* light = m_lightOwner.GetOrNull(rid);
	if (light != nullptr)
	{
		light->m_spotAttenuation = attenuation;
	}
}

RID RenderServer::CameraCreate() { return m_cameraOwner.CreateRID(); }

void RenderServer::CameraFree(RID camera)
{
	if (m_cameraOwner.GetOrNull(camera) == nullptr)
	{
		return;
	}

	// 1) Detach every Viewport that still renders with this camera.
	for (RID viewportRID : m_viewportOwner.GetRIDList())
	{
		ViewportData* viewportData = m_viewportOwner.GetOrNull(viewportRID);
		if (viewportData != nullptr && viewportData->m_camera == camera)
		{
			viewportData->m_camera = RID::Invalid;
		}
	}

	// 2) Release the camera independently of its viewports.
	m_cameraOwner.Free(camera);
}

void RenderServer::CameraSetTransform(RID camera, Matrix4x4 const& cameraToWorld)
{
	CameraData* cameraData = m_cameraOwner.GetOrNull(camera);
	if (cameraData != nullptr)
	{
		cameraData->m_cameraToWorld = cameraToWorld;
	}
}

void RenderServer::CameraSetPerspective(RID camera, float fovDegrees, float nearZ, float farZ)
{
	CameraData* cameraData = m_cameraOwner.GetOrNull(camera);
	if (cameraData != nullptr)
	{
		cameraData->m_mode       = CameraMode::Perspective;
		cameraData->m_fovDegrees = fovDegrees;
		cameraData->m_nearZ      = nearZ;
		cameraData->m_farZ       = farZ;
	}
}

void RenderServer::CameraSetOrthographic(RID camera, float size, float nearZ, float farZ)
{
	CameraData* cameraData = m_cameraOwner.GetOrNull(camera);
	if (cameraData != nullptr)
	{
		cameraData->m_mode  = CameraMode::Orthographic;
		cameraData->m_size  = size;
		cameraData->m_nearZ = nearZ;
		cameraData->m_farZ  = farZ;
	}
}

void RenderServer::Render()
{
	for (RID viewportRID : m_viewportOwner.GetRIDList())
	{
		ViewportData* viewportData = m_viewportOwner.GetOrNull(viewportRID);
		if (viewportData == nullptr || !viewportData->m_active)
		{
			continue;
		}

		CameraData* cameraData = m_cameraOwner.GetOrNull(viewportData->m_camera);

		PrepareViewportData(viewportData);
		for (RID layerRID : viewportData->m_canvasLayers)
		{
			PrepareCanvasLayerRequests(layerRID, viewportRID, viewportData);
		}

		m_renderer->ClearSceneTargets(viewportData);
		m_renderer->RenderViewport(viewportData, cameraData);
	}
}

void RenderServer::PrepareCanvasLayerRequests(RID layer, RID viewport, ViewportData* viewportData)
{
	CanvasLayerData* layerData = m_canvasLayerOwner.GetOrNull(layer);
	if (layerData == nullptr || layerData->m_viewport != viewport)
	{
		return;
	}
	for (RID itemRID : layerData->m_children)
	{
		PrepareCanvasItemRequests(itemRID, viewport, viewportData, Vector2::Zero);
	}
}

void RenderServer::PrepareCanvasItemRequests(
	RID itemRID, RID viewport, ViewportData* viewportData, Vector2 const& parentPosition)
{
	CanvasItemData* item = m_canvasItemOwner.GetOrNull(itemRID);
	if (item == nullptr || item->m_viewport != viewport || !item->m_visible)
	{
		return;
	}
	Vector2 const position = parentPosition + item->m_position;
	if (!item->m_commands.empty())
	{
		std::vector<Vertex> vertices;
		for (CanvasItemData::Command const* command : item->m_commands)
		{
			switch (command->type)
			{
			case CanvasItemData::Command::TYPE_RECT:
			{
				CanvasItemData::CommandRect const* rectCommand =
					static_cast<CanvasItemData::CommandRect const*>(command);
				AABB2 const& rect = rectCommand->rect;
				float const  x    = position.x;
				float const  y    = static_cast<float>(viewportData->m_outputResolution.y) - position.y;
				Vector3      a(x + rect.m_mins.x, y - rect.m_mins.y, 0.0f);
				Vector3      b(x + rect.m_maxs.x, y - rect.m_mins.y, 0.0f);
				Vector3      c(x + rect.m_maxs.x, y - rect.m_maxs.y, 0.0f);
				Vector3      d(x + rect.m_mins.x, y - rect.m_maxs.y, 0.0f);
				vertices.emplace_back(a, rectCommand->color, Vector2::Zero);
				vertices.emplace_back(b, rectCommand->color, Vector2::Zero);
				vertices.emplace_back(c, rectCommand->color, Vector2::Zero);
				vertices.emplace_back(a, rectCommand->color, Vector2::Zero);
				vertices.emplace_back(c, rectCommand->color, Vector2::Zero);
				vertices.emplace_back(d, rectCommand->color, Vector2::Zero);
				break;
			}
			}
		}
		if (!vertices.empty())
		{
			unsigned int const byteSize = static_cast<unsigned int>(vertices.size() * sizeof(Vertex));
			if (item->m_vertexBuffer == nullptr || item->m_vertexBuffer->GetSize() != byteSize)
			{
				delete item->m_vertexBuffer;
				item->m_vertexBuffer = m_renderer->CreateVertexBuffer(byteSize, sizeof(Vertex));
			}
			m_renderer->CopyCPUToGPU(vertices.data(), byteSize, item->m_vertexBuffer);

			RenderRequest request;
			request.m_pass           = RenderRequestPass::UI;
			request.m_vertexBuffer   = item->m_vertexBuffer;
			request.m_shader         = m_defaultUnlit->GetShader();
			request.m_blendMode      = BlendMode::ALPHA;
			request.m_depthMode      = DepthMode::READ_ONLY_ALWAYS;
			request.m_rasterizerMode = RasterizerMode::SOLID_CULL_NONE;
			viewportData->m_renderRequests[static_cast<size_t>(RenderRequestPass::UI)].push_back(request);
		}
	}
	for (RID childRID : item->m_children)
	{
		PrepareCanvasItemRequests(childRID, viewport, viewportData, position);
	}
}

bool RenderServer::OnWindowResized(EventArgs& args)
{
	int     width  = args.GetValue("width", 0);
	int     height = args.GetValue("height", 0);
	IntVec2 newDimensions(width, height);

	g_engine->m_renderServer->m_renderer->ResizeBackBuffer(newDimensions);

	return false;
}

Shader* RenderServer::CreateShader(std::string const& name, std::string const& source, std::string const& sourcePath)
{
	return m_renderer->CreateShader(name, source, sourcePath);
}

Ref<ShaderResource> RenderServer::GetBuiltinShaderResource(std::string const& name, std::string_view source)
{
	return m_renderer->GetBuiltinShaderResource(name, source);
}

Ref<ShaderResource> RenderServer::GetDefaultUnlitShaderResource() const { return m_defaultUnlit; }

Ref<ShaderResource> RenderServer::GetDefaultLitShaderResource() const { return m_defaultLit; }

Ref<MaterialResource> RenderServer::GetDefaultMaterialResource() const { return m_defaultMaterial; }

GPUTexture* RenderServer::CreateGPUTexture(char const* name, IntVec2 dimensions, int bytesPerTexel, uint8_t const* data)
{
	return m_renderer->CreateGPUTexture(name, dimensions, bytesPerTexel, data);
}

void RenderServer::DestroyTexture(GPUTexture* texture) { m_renderer->DestroyTexture(texture); }

VertexBuffer* RenderServer::CreateVertexBuffer(unsigned int size, unsigned int stride)
{
	return m_renderer->CreateVertexBuffer(size, stride);
}

VertexBuffer* RenderServer::CreateVertexBuffer(void const* data, unsigned int size, unsigned int stride)
{
	return m_renderer->CreateVertexBuffer(data, size, stride);
}

IndexBuffer* RenderServer::CreateIndexBuffer(unsigned int size) { return m_renderer->CreateIndexBuffer(size); }

IndexBuffer* RenderServer::CreateIndexBuffer(void const* data, unsigned int size, unsigned int stride)
{
	return m_renderer->CreateIndexBuffer(data, size, stride);
}

void RenderServer::UpdateVertexBuffer(VertexBuffer* buffer, void const* data, unsigned int size)
{
	m_renderer->UpdateVertexBuffer(buffer, data, size);
}

void RenderServer::CopyCPUToGPU(void const* data, unsigned int size, VertexBuffer* buffer)
{
	m_renderer->CopyCPUToGPU(data, size, buffer);
}

bool RenderServer::InitImGui() { return m_renderer->InitImGui(); }

void RenderServer::ShutdownImGui() { m_renderer->ShutdownImGui(); }

void RenderServer::BeginImGuiFrame() { m_renderer->BeginImGuiFrame(); }

void RenderServer::RenderImGui(ImDrawData* drawData)
{
	m_renderer->BindBackBuffer();
	m_renderer->RenderImGui(drawData);
}

ImTextureID RenderServer::GetImGuiTextureID(GPUTexture* texture) const
{
	return m_renderer->GetImGuiTextureID(texture);
}

RID RenderServer::CanvasItemCreate() { return m_canvasItemOwner.CreateRID(); }

void RenderServer::CanvasItemFree(RID rid)
{
	CanvasItemData* item = m_canvasItemOwner.GetOrNull(rid);
	if (item == nullptr)
	{
		return;
	}

	CanvasItemSetParent(rid, RID::Invalid);
	for (RID childRID : item->m_children)
	{
		CanvasItemData* child = m_canvasItemOwner.GetOrNull(childRID);
		if (child != nullptr)
		{
			child->m_parent = RID::Invalid;
		}
	}
	delete item->m_vertexBuffer;
	item->m_vertexBuffer = nullptr;
	m_canvasItemOwner.Free(rid);
}

void RenderServer::CanvasItemSetViewport(RID rid, RID viewport)
{
	CanvasItemData* item = m_canvasItemOwner.GetOrNull(rid);
	if (item != nullptr && (!viewport.IsValid() || m_viewportOwner.GetOrNull(viewport) != nullptr))
	{
		if (item->m_viewport != viewport)
		{
			CanvasItemSetParent(rid, RID::Invalid);
		}
		item->m_viewport = viewport;
	}
}

void RenderServer::CanvasItemSetParent(RID rid, RID parent)
{
	CanvasItemData* item = m_canvasItemOwner.GetOrNull(rid);
	if (item == nullptr)
	{
		return;
	}
	CanvasItemData* parentItem = m_canvasItemOwner.GetOrNull(parent);
	if (parent.IsValid() && (parentItem == nullptr || parent == rid || parentItem->m_viewport != item->m_viewport))
	{
		return;
	}
	for (RID ancestor = parent; ancestor.IsValid();)
	{
		if (ancestor == rid)
		{
			return;
		}
		CanvasItemData* ancestorItem = m_canvasItemOwner.GetOrNull(ancestor);
		ancestor                     = ancestorItem != nullptr ? ancestorItem->m_parent : RID::Invalid;
	}

	if (CanvasItemData* oldParent = m_canvasItemOwner.GetOrNull(item->m_parent))
	{
		auto& children = oldParent->m_children;
		children.erase(std::remove(children.begin(), children.end(), rid), children.end());
	}
	if (CanvasLayerData* oldLayer = m_canvasLayerOwner.GetOrNull(item->m_layer))
	{
		auto& children = oldLayer->m_children;
		children.erase(std::remove(children.begin(), children.end(), rid), children.end());
	}
	item->m_parent = RID::Invalid;
	item->m_layer  = RID::Invalid;
	if (parentItem != nullptr)
	{
		item->m_parent = parent;
		parentItem->m_children.push_back(rid);
	}
}

void RenderServer::CanvasItemSetPosition(RID rid, Vector2 const& position)
{
	CanvasItemData* item = m_canvasItemOwner.GetOrNull(rid);
	if (item != nullptr)
	{
		item->m_position = position;
	}
}

void RenderServer::CanvasItemSetVisible(RID rid, bool visible)
{
	CanvasItemData* item = m_canvasItemOwner.GetOrNull(rid);
	if (item != nullptr)
	{
		item->m_visible = visible;
	}
}

void RenderServer::CanvasItemClear(RID rid)
{
	CanvasItemData* item = m_canvasItemOwner.GetOrNull(rid);
	if (item != nullptr)
	{
		item->ClearCommands();
	}
}

void RenderServer::CanvasItemAddRect(RID rid, AABB2 const& rect, Color const& color)
{
	CanvasItemData* item = m_canvasItemOwner.GetOrNull(rid);
	if (item == nullptr || rect.GetWidth() <= 0 || rect.GetHeight() <= 0)
	{
		return;
	}

	CanvasItemData::CommandRect* command = new CanvasItemData::CommandRect();
	command->rect                        = rect;
	command->color                       = color;
	item->m_commands.push_back(command);
}

RID RenderServer::CanvasLayerCreate() { return m_canvasLayerOwner.CreateRID(); }

void RenderServer::CanvasLayerFree(RID layer)
{
	CanvasLayerData* layerData = m_canvasLayerOwner.GetOrNull(layer);
	if (layerData == nullptr)
	{
		return;
	}
	CanvasLayerSetViewport(layer, RID::Invalid);
	for (RID childRID : layerData->m_children)
	{
		CanvasItemData* child = m_canvasItemOwner.GetOrNull(childRID);
		if (child != nullptr)
		{
			child->m_layer = RID::Invalid;
		}
	}
	m_canvasLayerOwner.Free(layer);
}

void RenderServer::CanvasLayerSetViewport(RID layer, RID viewport)
{
	CanvasLayerData* layerData = m_canvasLayerOwner.GetOrNull(layer);
	if (layerData == nullptr || (viewport.IsValid() && m_viewportOwner.GetOrNull(viewport) == nullptr))
	{
		return;
	}
	if (ViewportData* oldViewport = m_viewportOwner.GetOrNull(layerData->m_viewport))
	{
		auto& layers = oldViewport->m_canvasLayers;
		layers.erase(std::remove(layers.begin(), layers.end(), layer), layers.end());
	}
	layerData->m_viewport = viewport;
	if (ViewportData* newViewport = m_viewportOwner.GetOrNull(viewport))
	{
		newViewport->m_canvasLayers.push_back(layer);
	}
}

void RenderServer::CanvasLayerAddChild(RID layer, RID child)
{
	CanvasLayerData* layerData = m_canvasLayerOwner.GetOrNull(layer);
	CanvasItemData*  item      = m_canvasItemOwner.GetOrNull(child);
	if (layerData != nullptr && item != nullptr && layerData->m_viewport.IsValid()
		&& layerData->m_viewport == item->m_viewport)
	{
		CanvasItemSetParent(child, RID::Invalid);
		item->m_layer = layer;
		layerData->m_children.push_back(child);
	}
}

void RenderServer::CanvasLayerRemoveChild(RID layer, RID child)
{
	CanvasLayerData* layerData = m_canvasLayerOwner.GetOrNull(layer);
	CanvasItemData*  item      = m_canvasItemOwner.GetOrNull(child);
	if (layerData != nullptr && item != nullptr && item->m_layer == layer)
	{
		item->m_layer  = RID::Invalid;
		auto& children = layerData->m_children;

		// std::remove will move the matching child to the end of the vector and return an iterator to the new end.
		// Which basically means remove all the elements that equal to child
		children.erase(std::remove(children.begin(), children.end(), child), children.end());
	}
}

RID RenderServer::InstanceCreate() { return m_instanceOwner.CreateRID(); }

void RenderServer::InstanceFree(RID instance)
{
	Instance const* data = m_instanceOwner.GetOrNull(instance);
	if (data == nullptr)
	{
		return;
	}

	// 1) Leave the Scenario list so Render() stops visiting this RID.
	// 2) Free the storage afterwards.
	ScenarioData* scenarioData = m_scenarioOwner.GetOrNull(data->m_scenario);
	if (scenarioData != nullptr)
	{
		auto const foundInstance =
			std::find(scenarioData->m_instances.begin(), scenarioData->m_instances.end(), instance);
		if (foundInstance != scenarioData->m_instances.end())
		{
			scenarioData->m_instances.erase(foundInstance);
		}
	}

	m_instanceOwner.Free(instance);
}

void RenderServer::InstanceSetTransform(RID instance, Matrix4x4 const& transform)
{
	Instance* data = m_instanceOwner.GetOrNull(instance);
	if (data != nullptr)
	{
		data->m_transform = transform;
	}
}

void RenderServer::InstanceSetVisible(RID instance, bool visible)
{
	Instance* data = m_instanceOwner.GetOrNull(instance);
	if (data != nullptr)
	{
		data->m_visible = visible;
	}
}

void RenderServer::PrepareViewportData(ViewportData* viewportData)
{
	if (viewportData == nullptr)
	{
		return;
	}

	// 1) Requests and lights only describe the current frame.
	viewportData->m_directionalLight = {};
	viewportData->m_pointLights.clear();
	for (auto& requests : viewportData->m_renderRequests)
	{
		requests.clear();
	}

	RID           scenarioRID  = viewportData->m_scenario;
	ScenarioData* scenarioData = m_scenarioOwner.GetOrNull(scenarioRID);

	if (scenarioData == nullptr)
	{
		return;
	}

	// 2) Every instance of the Scenario contributes a request or a light.
	for (RID instanceRID : scenarioData->m_instances)
	{
		Instance* instance = m_instanceOwner.GetOrNull(instanceRID);
		if (instance == nullptr)
		{
			continue;
		}

		switch (instance->m_baseType)
		{
		case InstanceBaseType::None:
			continue;
		case InstanceBaseType::Mesh:
		{
			RenderRequest request = BuildInstanceRenderRequest(instance);
			if (!request.IsValid())
			{
				continue;
			}
			viewportData->m_renderRequests[static_cast<size_t>(request.m_pass)].push_back(request);
			break;
		}
		case InstanceBaseType::Light:
		{
			LightData* lightData = m_lightOwner.GetOrNull(instance->m_base);

			// Lights of invisible instances are dropped like their mesh counterparts.
			// e.g. SetVisible(false) on a Light3D removes its contribution for this frame
			if (lightData == nullptr || !instance->m_visible)
			{
				continue;
			}

			// Only the first directional light and kMaxPointLights positional lights reach the shader.
			// e.g. further lights of the Scene stay without a GPU counterpart this frame
			if (lightData->m_type == LightType::Directional)
			{
				if (viewportData->m_directionalLight.m_instance == nullptr)
				{
					viewportData->m_directionalLight = { instance, lightData };
				}
				break;
			}

			if (viewportData->m_pointLights.size() < static_cast<size_t>(kMaxPointLights))
			{
				viewportData->m_pointLights.push_back({ instance, lightData });
			}
			break;
		}
		}
	}
}

RenderRequest RenderServer::BuildInstanceRenderRequest(Instance const* instance)
{
	RenderRequest request;

	// 1) Invisible instances never produce a request.
	// 2) The Base type decides how the instance is drawn.
	// 3) The mesh material fills shader, textures, tint and draw states.
	if (!instance->m_visible)
	{
		return request;
	}

	switch (instance->m_baseType)
	{
	case InstanceBaseType::Mesh:
	{
		MeshData* meshData = m_meshOwner.GetOrNull(instance->m_base);
		if (meshData == nullptr)
		{
			return request;
		}

		VertexBufferData* vertexBufferData = m_vertexBufferOwner.GetOrNull(meshData->m_vertexBufferRID);
		if (vertexBufferData == nullptr)
		{
			return request;
		}

		request.m_vertexBuffer = vertexBufferData->m_vertexBuffer;

		if (meshData->m_indexBufferRID != RID::Invalid)
		{
			IndexBufferData* indexBufferData = m_indexBufferOwner.GetOrNull(meshData->m_indexBufferRID);
			if (indexBufferData == nullptr)
			{
				return request;
			}

			request.m_indexBuffer = indexBufferData->m_indexBuffer;
		}

		// Meshes without a material of their own fall back to the shared default material.
		MaterialResource* materialResource = meshData->m_materialResource.IsValid()
												 ? meshData->m_materialResource.Get()
												 : GetDefaultMaterialResource().Get();

		if (materialResource != nullptr)
		{
			ApplyMaterialToRequest(*materialResource, request);
		}
		break;
	}
	case InstanceBaseType::None:
		break;
	default:
		break;
	}

	request.m_modelToWorld = instance->m_transform;

	return request;
}

void RenderServer::ApplyMaterialToRequest(MaterialResource const& materialResource, RenderRequest& request)
{
	// 1) Missing texture slots stay nullptr so ExecuteRenderRequest() binds the default textures.
	// 2) A missing shader is replaced here, because the backend rejects a null one when drawing.
	for (size_t slot = 0; slot < SurfaceTextureSlot::Count; ++slot)
	{
		Ref<TextureResource> const& textureResource = materialResource.m_textureResources[slot];
		if (!textureResource.IsValid())
		{
			continue;
		}

		TextureData* textureData = m_textureOwner.GetOrNull(textureResource->GetTextureRID());
		request.m_textures[slot] = textureData != nullptr ? textureData->m_texture : nullptr;
	}

	if (materialResource.m_shaderResource.IsValid())
	{
		request.m_shader = materialResource.m_shaderResource->GetShader();
	}
	else
	{
		// e.g. a material that only overrides tint still needs a shader to draw with
		request.m_shader = m_defaultUnlit->GetShader();
	}

	request.m_tint           = materialResource.m_tint;
	request.m_blendMode      = materialResource.m_blendMode;
	request.m_depthMode      = materialResource.m_depthMode;
	request.m_rasterizerMode = materialResource.m_rasterizerMode;
	request.m_samplerMode    = materialResource.m_samplerMode;
}

RID RenderServer::RegisterVertexBuffer(VertexBuffer* vertexBuffer)
{
	RID               vertexBufferRID = m_vertexBufferOwner.CreateRID();
	VertexBufferData* data            = m_vertexBufferOwner.GetOrNull(vertexBufferRID);
	data->m_vertexBuffer              = vertexBuffer;

	return vertexBufferRID;
}

void RenderServer::FreeVertexBuffer(RID vertexBuffer)
{
	VertexBufferData* data = m_vertexBufferOwner.GetOrNull(vertexBuffer);
	if (data == nullptr)
	{
		return;
	}

	delete data->m_vertexBuffer;
	data->m_vertexBuffer = nullptr;

	m_vertexBufferOwner.Free(vertexBuffer);
}

RID RenderServer::RegisterIndexBuffer(IndexBuffer* indexBuffer)
{
	RID              indexBufferRID = m_indexBufferOwner.CreateRID();
	IndexBufferData* data           = m_indexBufferOwner.GetOrNull(indexBufferRID);
	data->m_indexBuffer             = indexBuffer;

	return indexBufferRID;
}

void RenderServer::FreeIndexBuffer(RID indexBuffer)
{
	IndexBufferData* data = m_indexBufferOwner.GetOrNull(indexBuffer);
	if (data == nullptr)
	{
		return;
	}

	delete data->m_indexBuffer;
	data->m_indexBuffer = nullptr;

	m_indexBufferOwner.Free(indexBuffer);
}

bool RenderServer::PrepareMeshData(MeshData& data, MeshResource const& meshResource)
{
	// 1) Nothing can be uploaded while the Renderer is down.
	// 2) A refresh must not keep the GPU buffers of the previous upload alive.
	if (!m_started)
	{
		return false;
	}

	FreeVertexBuffer(data.m_vertexBufferRID);
	FreeIndexBuffer(data.m_indexBufferRID);
	data.m_vertexBufferRID  = RID::Invalid;
	data.m_indexBufferRID   = RID::Invalid;
	data.m_materialResource = meshResource.m_materialResource;

	if (meshResource.IsEmpty())
	{
		return false;
	}

	VertexBuffer* vertexBuffer = m_renderer->CreateVertexBuffer(
		meshResource.m_vertices.data(),
		meshResource.m_vertexCount * meshResource.m_vertexStride,
		meshResource.m_vertexStride);

	if (vertexBuffer == nullptr)
	{
		return false;
	}

	data.m_vertexBufferRID = RegisterVertexBuffer(vertexBuffer);

	IndexBuffer* indexBuffer = m_renderer->CreateIndexBuffer(
		meshResource.m_indices.data(),
		meshResource.m_indexCount * meshResource.m_indexStride,
		meshResource.m_indexStride);

	if (indexBuffer == nullptr)
	{
		FreeVertexBuffer(data.m_vertexBufferRID);
		data.m_vertexBufferRID = RID::Invalid;
		return false;
	}

	data.m_indexBufferRID = RegisterIndexBuffer(indexBuffer);

	return true;
}

RID RenderServer::MeshCreate(MeshResource const& meshResource)
{
	if (!m_started)
	{
		return RID::Invalid;
	}

	RID       meshRID = m_meshOwner.CreateRID();
	MeshData* data    = m_meshOwner.GetOrNull(meshRID);

	if (!PrepareMeshData(*data, meshResource))
	{
		m_meshOwner.Free(meshRID);
		return RID::Invalid;
	}

	return meshRID;
}

void RenderServer::MeshRefresh(RID mesh, MeshResource const& meshResource)
{
	MeshData* data = m_meshOwner.GetOrNull(mesh);
	if (data != nullptr)
	{
		PrepareMeshData(*data, meshResource);
	}
}

void RenderServer::MeshFree(RID mesh)
{
	MeshData* data = m_meshOwner.GetOrNull(mesh);
	if (data == nullptr)
	{
		return;
	}

	// 1) Every instance that points at this Mesh RID loses its Base.
	// 2) The switch keeps other Base types untouched when they are added later.
	for (RID rid : m_instanceOwner.GetRIDList())
	{
		Instance* instance = m_instanceOwner.GetOrNull(rid);
		switch (instance->m_baseType)
		{
		case InstanceBaseType::Mesh:
			if (instance->m_base == mesh)
			{
				instance->m_base     = RID::Invalid;
				instance->m_baseType = InstanceBaseType::None;
			}
			break;
		case InstanceBaseType::None:
		default:
			break;
		}
	}

	// 3) The GPU buffers belong to this registration, so they are released with it.
	FreeVertexBuffer(data->m_vertexBufferRID);
	FreeIndexBuffer(data->m_indexBufferRID);
	data->m_vertexBufferRID = RID::Invalid;
	data->m_indexBufferRID  = RID::Invalid;

	m_meshOwner.Free(mesh);
}

bool RenderServer::PrepareTextureData(TextureData& data, TextureResource const& textureResource)
{
	// 1) Nothing can be uploaded while the Renderer is down.
	// 2) A refresh must not keep the GPUTexture of the previous upload alive.
	if (!m_started)
	{
		return false;
	}

	if (data.m_texture != nullptr)
	{
		m_renderer->DestroyTexture(data.m_texture);
		data.m_texture = nullptr;
	}

	if (textureResource.IsEmpty())
	{
		return false;
	}

	data.m_texture = m_renderer->CreateGPUTexture(
		textureResource.GetName().ToUtf8().c_str(),
		textureResource.GetDimensions(),
		textureResource.GetChannels(),
		textureResource.GetImage()->GetRawData());

	return data.m_texture != nullptr;
}

RID RenderServer::TextureCreate(TextureResource const& textureResource)
{
	if (!m_started)
	{
		return RID::Invalid;
	}

	RID          textureRID = m_textureOwner.CreateRID();
	TextureData* data       = m_textureOwner.GetOrNull(textureRID);

	if (!PrepareTextureData(*data, textureResource))
	{
		m_textureOwner.Free(textureRID);
		return RID::Invalid;
	}

	return textureRID;
}

void RenderServer::TextureRefresh(RID texture, TextureResource const& textureResource)
{
	TextureData* data = m_textureOwner.GetOrNull(texture);
	if (data != nullptr)
	{
		PrepareTextureData(*data, textureResource);
	}
}

void RenderServer::TextureFree(RID texture)
{
	TextureData* data = m_textureOwner.GetOrNull(texture);
	if (data == nullptr)
	{
		return;
	}

	// GPU resources can only be released while the Renderer is alive.
	if (m_started && data->m_texture != nullptr)
	{
		m_renderer->DestroyTexture(data->m_texture);
	}
	data->m_texture = nullptr;

	m_textureOwner.Free(texture);
}

RID RenderServer::ScenarioCreate() { return m_scenarioOwner.CreateRID(); }

void RenderServer::ScenarioFree(RID scenario)
{
	if (m_scenarioOwner.GetOrNull(scenario) == nullptr)
	{
		return;
	}

	for (RID rid : m_instanceOwner.GetRIDList())
	{
		Instance* instance = m_instanceOwner.GetOrNull(rid);
		if (instance->m_scenario == scenario)
		{
			instance->m_scenario = RID::Invalid;
		}
	}

	for (RID rid : m_viewportOwner.GetRIDList())
	{
		ViewportData* viewport = m_viewportOwner.GetOrNull(rid);
		if (viewport->m_scenario == scenario)
		{
			viewport->m_scenario = RID::Invalid;
		}
	}

	m_scenarioOwner.Free(scenario);
}

void RenderServer::InstanceSetBase(RID instance, RID base)
{
	Instance* data = m_instanceOwner.GetOrNull(instance);
	if (data == nullptr)
	{
		return;
	}

	// A Base RID defines the instance type only while its owner still holds the RID.
	InstanceBaseType baseType = InstanceBaseType::None;
	if (base != RID::Invalid && m_meshOwner.GetOrNull(base) != nullptr)
	{
		baseType = InstanceBaseType::Mesh;
	}
	else if (base != RID::Invalid && m_lightOwner.GetOrNull(base) != nullptr)
	{
		baseType = InstanceBaseType::Light;
	}

	switch (baseType)
	{
	case InstanceBaseType::Mesh:
	case InstanceBaseType::Light:
		data->m_base = base;
		break;
	case InstanceBaseType::None:
	default:
		data->m_base = RID::Invalid;
		break;
	}

	data->m_baseType = baseType;
}

void RenderServer::InstanceSetScenario(RID instance, RID scenario)
{
	Instance* data = m_instanceOwner.GetOrNull(instance);
	if (data == nullptr)
	{
		return;
	}

	if (scenario != RID::Invalid && m_scenarioOwner.GetOrNull(scenario) == nullptr)
	{
		return;
	}

	// 1) Leave the previous Scenario list.
	// 2) Join the new one so Render() visits this instance again.
	ScenarioData* oldScenarioData = m_scenarioOwner.GetOrNull(data->m_scenario);
	if (oldScenarioData != nullptr)
	{
		auto const foundInstance =
			std::find(oldScenarioData->m_instances.begin(), oldScenarioData->m_instances.end(), instance);
		if (foundInstance != oldScenarioData->m_instances.end())
		{
			oldScenarioData->m_instances.erase(foundInstance);
		}
	}

	data->m_scenario = scenario;

	ScenarioData* newScenarioData = m_scenarioOwner.GetOrNull(scenario);
	if (newScenarioData != nullptr)
	{
		newScenarioData->m_instances.push_back(instance);
	}
}

void RenderServer::ViewportSetScenario(RID viewport, RID scenario)
{
	ViewportData* data = m_viewportOwner.GetOrNull(viewport);
	if (data == nullptr)
	{
		return;
	}

	if (scenario != RID::Invalid && m_scenarioOwner.GetOrNull(scenario) == nullptr)
	{
		return;
	}

	data->m_scenario = scenario;
}
