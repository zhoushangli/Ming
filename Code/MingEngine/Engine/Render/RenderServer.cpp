#include "MingEngine/Engine/Render/RenderServer.hpp"

#include "MingEngine/Core/Memory.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/Renderer.hpp"

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

	// 1) GPU resources can only be released while the Renderer is alive.
	// 2) Drop the RID from the owner before its storage is freed.
	if (m_started)
	{
		m_renderer->DestroyViewportResources(*viewportData);
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
	m_renderer->ResizeViewport(*viewportData, size);
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

	// 2) Clear lights until per-viewport collection is implemented.
	viewportData->m_lights.clear();
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

	viewportData->m_renderRequests[static_cast<size_t>(request.m_pass)].push_back(request);
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

		// 1) Only the Scenario bound to this Viewport contributes instances.
		// 2) A missing camera still renders UI, but the world passes have no viewpoint.
		CameraData const* cameraData   = m_cameraOwner.GetOrNull(viewportData->m_camera);
		ScenarioData*     scenarioData = m_scenarioOwner.GetOrNull(viewportData->m_scenario);
		if (scenarioData != nullptr && cameraData != nullptr)
		{
			for (RID instanceRID : scenarioData->m_instances)
			{
				Instance const* instance = m_instanceOwner.GetOrNull(instanceRID);
				if (instance == nullptr)
				{
					continue;
				}

				RenderRequest request = BuildInstanceRenderRequest(*instance);
				if (request.IsValid())
				{
					ViewportSubmitRenderRequest(viewportRID, request);
				}
			}
		}

		m_renderer->ClearSceneTargets(*viewportData);
		m_renderer->SetViewport(viewportData->m_outputResolution);
		m_renderer->RenderViewport(*viewportData, cameraData);
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

void RenderServer::InstanceSetTint(RID instance, Color tint)
{
	Instance* data = m_instanceOwner.GetOrNull(instance);
	if (data != nullptr)
	{
		data->m_tint = tint;
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

RenderRequest RenderServer::BuildInstanceRenderRequest(Instance const& instance)
{
	RenderRequest request;

	// 1) Invisible instances never produce a request.
	// 2) The Base type decides how the instance is drawn.
	if (!instance.m_visible)
	{
		return request;
	}

	switch (instance.m_baseType)
	{
	case InstanceBaseType::Mesh:
		// MeshData only marks the RID as registered, so there are no GPU handles yet.
		// The request stays empty until the mesh render path stores buffers and shaders again.
		break;
	case InstanceBaseType::None:
	default:
		break;
	}

	return request;
}

RID RenderServer::MeshCreate(Ref<MeshResource> const& mesh)
{
	if (!mesh.IsValid())
	{
		return RID::Invalid;
	}

	// The MeshResource is not retained here: MeshData only marks the RID as registered,
	// so the scene keeps owning the resource and unloading is never blocked by the RenderServer.
	return m_meshOwner.CreateRID();
}

void RenderServer::MeshFree(RID mesh)
{
	if (m_meshOwner.GetOrNull(mesh) == nullptr)
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

	m_meshOwner.Free(mesh);
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