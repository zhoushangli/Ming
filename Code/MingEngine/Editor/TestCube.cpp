#include "MingEngine/Editor/TestCube.hpp"

#include "MingEngine/Editor/EditorNode.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/Rgba8.hpp"
#include "MingEngine/Engine/Render/Renderer.hpp"
#include "MingEngine/Engine/Render/VertexBuffer.hpp"

namespace
{
void AddQuad(std::vector<Vertex>& verts, Vec3 const& a, Vec3 const& b, Vec3 const& c, Vec3 const& d, Rgba8 const& color)
{
	verts.push_back(Vertex(a, color));
	verts.push_back(Vertex(b, color));
	verts.push_back(Vertex(c, color));
	verts.push_back(Vertex(a, color));
	verts.push_back(Vertex(c, color));
	verts.push_back(Vertex(d, color));
}

void AddBox(std::vector<Vertex>& verts, Vec3 const& mins, Vec3 const& maxs, Rgba8 const& color)
{
	Vec3 const bsw(mins.x, mins.y, mins.z);
	Vec3 const bse(maxs.x, mins.y, mins.z);
	Vec3 const bne(maxs.x, maxs.y, mins.z);
	Vec3 const bnw(mins.x, maxs.y, mins.z);
	Vec3 const tsw(mins.x, mins.y, maxs.z);
	Vec3 const tse(maxs.x, mins.y, maxs.z);
	Vec3 const tne(maxs.x, maxs.y, maxs.z);
	Vec3 const tnw(mins.x, maxs.y, maxs.z);

	AddQuad(verts, bsw, bse, bne, bnw, color);
	AddQuad(verts, tsw, tnw, tne, tse, color);
	AddQuad(verts, bsw, tsw, tse, bse, color);
	AddQuad(verts, bse, tse, tne, bne, color);
	AddQuad(verts, bne, tne, tnw, bnw, color);
	AddQuad(verts, bnw, tnw, tsw, bsw, color);
}
} // namespace

TestCube::TestCube()
{
	float constexpr halfExtent = 0.5f;
	float constexpr halfWidth  = 0.035f;
	Rgba8 const color(80, 220, 255, 255);

	for (float y : { -halfExtent, halfExtent })
	{
		for (float z : { -halfExtent, halfExtent })
		{
			AddBox(m_renderVerts,
				Vec3(-halfExtent, y - halfWidth, z - halfWidth),
				Vec3(halfExtent, y + halfWidth, z + halfWidth),
				color);
		}
	}

	for (float x : { -halfExtent, halfExtent })
	{
		for (float z : { -halfExtent, halfExtent })
		{
			AddBox(m_renderVerts,
				Vec3(x - halfWidth, -halfExtent, z - halfWidth),
				Vec3(x + halfWidth, halfExtent, z + halfWidth),
				color);
		}
	}

	for (float x : { -halfExtent, halfExtent })
	{
		for (float y : { -halfExtent, halfExtent })
		{
			AddBox(m_renderVerts,
				Vec3(x - halfWidth, y - halfWidth, -halfExtent),
				Vec3(x + halfWidth, y + halfWidth, halfExtent),
				color);
		}
	}

	m_renderVertexBuffer = g_engine->m_renderer->CreateVertexBuffer(m_renderVerts);
}

TestCube::~TestCube()
{
	if (m_renderVertexBuffer != nullptr)
	{
		delete m_renderVertexBuffer;
		m_renderVertexBuffer = nullptr;
	}
}

void TestCube::OnReady()
{
	Node3D::OnReady();
	if (EditorNode* editorNode = EditorNode::Get())
	{
		editorNode->RegisterSelectableMesh(m_renderVerts, GetHandle());
	}
}

void TestCube::OnExitTree()
{
	if (EditorNode* editorNode = EditorNode::Get())
	{
		editorNode->UnregisterSelectable(GetHandle());
	}
	Node3D::OnExitTree();
}

RenderRequest TestCube::SubmitRenderRequest() const
{
	RenderRequest request;
	if (m_renderVerts.empty() || g_engine == nullptr || g_engine->m_renderer == nullptr)
	{
		return request;
	}

	request.m_pass           = RenderRequestPass::Opaque;
	request.m_modelToWorld   = GetWorldTransform();
	request.m_tint           = Rgba8::White;
	request.m_vertexBuffer   = m_renderVertexBuffer;
	request.m_indexBuffer    = nullptr;
	request.m_shader         = nullptr;
	return request;
}
