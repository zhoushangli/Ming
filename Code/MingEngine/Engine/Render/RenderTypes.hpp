#pragma once

#include "MingEngine/Core/Math/Matrix4x4.hpp"
#include "MingEngine/Core/Math/Vector2.hpp"
#include "MingEngine/Core/Math/Vector3.hpp"

#include <cstddef>
#include <cstdint>

enum class BlendMode
{
	ALPHA,
	ADDITIVE,
	OPAQUE,
	COUNT
};

enum class SamplerMode
{
	POINT_CLAMP,
	BILINEAR_CLAMP,
	POINT_WRAP,
	BILINEAR_WRAP,
	COUNT
};

enum class RasterizerMode
{
	SOLID_CULL_NONE,
	SOLID_CULL_BACK,
	WIREFRAME_CULL_NONE,
	WIREFRAME_CULL_BACK,
	COUNT
};

enum class DepthMode
{
	DISABLED,
	READ_ONLY_ALWAYS,
	READ_ONLY_LESS_EQUAL,
	READ_WRITE_LESS_EQUAL,
	COUNT
};

struct RendererServerConfig
{
	bool m_isEnable = true;
};

struct GPUDirectionalLight
{
	Vector3 m_direction;
	float   m_intensity;
	Vector3 m_color;
	float   m_padding;
};

struct GPUOmniLight
{
	Vector3 m_position;
	float   m_range;
	Vector3 m_color;
	float   m_intensity;
	float   m_attenuation;
	float   m_padding[3];
};
static const int kMaxPointLights = 16;

struct GPUSpotLight
{
	Vector3 m_position;
	float   m_range;
	Vector3 m_direction;
	float   m_intensity;
	Vector3 m_color;
	float   m_attenuation;
	float   m_spotAngle;
	float   m_spotAttenuation;
	float   m_padding[2];
};
static const int kMaxSpotLights = 16;

struct LightConstants
{
	GPUDirectionalLight m_directionalLight;
	int                 m_pointLightCount;
	int                 m_spotLightCount;
	float               m_padding[2]; // Pad to 16 bytes for array alignment
	GPUOmniLight        m_pointLights[kMaxPointLights];
	GPUSpotLight        m_spotLights[kMaxSpotLights];
};
static const int kLightConstantsSlot = 1;

struct CameraConstants
{
	Matrix4x4 m_worldToCameraTransform;
	Matrix4x4 m_cameraToRenderTransform;
	Matrix4x4 m_renderToClipTransform;
	Matrix4x4 m_cameraToWorldTransform;
	Matrix4x4 m_clipToCameraTransform;
};
static const int kCameraConstantsSlot = 2;

struct ModelConstants
{
	Matrix4x4 m_modelToWorld;
	float     m_modelColor[4];
};
static const int kModelConstantsSlot = 3;

struct PostProcessConstants
{
	Vector2 m_screenDimensions;
	float   m_cameraNear;
	float   m_cameraFar;
};
static const int kPostProcessConstantsSlot = 4;

struct FrameConstants
{
	float m_time;
	float m_deltaSeconds;
	float m_padding[2];
};
static const int kFrameConstantsSlot = 5;

enum class BuiltinConstantBufferType
{
	Light,
	Camera,
	Model,
	PostProcess,
	Frame,
	Count
};

struct BuiltinConstantBufferDesc
{
	char const* name = nullptr;
	size_t      size = 0;
	int         slot = 0;
};

static BuiltinConstantBufferDesc const BuiltinConstantBufferDescs[] = {
	{ "Light", sizeof(LightConstants), kLightConstantsSlot },
	{ "Camera", sizeof(CameraConstants), kCameraConstantsSlot },
	{ "Model", sizeof(ModelConstants), kModelConstantsSlot },
	{ "PostProcess", sizeof(PostProcessConstants), kPostProcessConstantsSlot },
	{ "Frame", sizeof(FrameConstants), kFrameConstantsSlot },
};
