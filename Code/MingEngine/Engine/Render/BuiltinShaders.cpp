#include "MingEngine/Engine/Render/BuiltinShaders.hpp"

namespace BuiltinShaders
{
std::string_view const DefaultLit = R"MING_SHADER(#define COMMON_ENABLE_SURFACE_TEXTURES
#ifndef COMMON_BUFFERS_HLSLI
#define COMMON_BUFFERS_HLSLI

cbuffer CameraConstants : register(b2)
{
    float4x4 WorldToCameraTransform;
    float4x4 CameraToRenderTransform;
    float4x4 RenderToClipTransform;
    float4x4 CameraToWorldTransform;
    float4x4 ClipToCameraTransform;
};

cbuffer ModelConstants : register(b3)
{
    float4x4 ModelToWorldTransform;
    float4 ModelColor;
};

cbuffer PostProcessConstants : register(b4)
{
    float2 ScreenSize;
    float CameraNear;
    float CameraFar;
};

cbuffer FrameConstants : register(b5)
{
    float Time;
    float DeltaSeconds;
    float2 FramePadding;
};

#ifdef COMMON_ENABLE_SURFACE_TEXTURES
Texture2D DiffuseTexture : register(t0);
SamplerState DiffuseSampler : register(s0);
#endif

#ifdef COMMON_ENABLE_POSTPROCESS_TEXTURES
Texture2D ColorTexture : register(t0);
Texture2D DepthTexture : register(t1);
Texture2D NormalTexture : register(t2);
SamplerState ColorSampler : register(s0);
SamplerState DepthSampler : register(s1);
SamplerState NormalSampler : register(s2);
#endif

#endif

#ifndef COMMON_LIGHTING_HLSLI
#define COMMON_LIGHTING_HLSLI

#ifndef MAX_POINT_LIGHTS
#define MAX_POINT_LIGHTS 16
#endif

#ifndef MAX_SPOT_LIGHTS
#define MAX_SPOT_LIGHTS 16
#endif

struct DirectionalLight
{
    float3 Direction;
    float Intensity;
    float3 Color;
    float Padding;
};

struct OmniLight
{
    float3 Position;
    float Range;
    float3 Color;
    float Intensity;
    float Attenuation;
    float3 Padding;
};

struct SpotLight
{
    float3 Position;
    float Range;
    float3 Direction;
    float Intensity;
    float3 Color;
    float Attenuation;
    float SpotAngle;
    float SpotAttenuation;
    float2 Padding;
};

cbuffer LightConstants : register(b1)
{
    DirectionalLight directionalLight;
    int NumPointLights;
    int NumSpotLights;
    float2 Padding;
    OmniLight PointLights[MAX_POINT_LIGHTS];
    SpotLight SpotLights[MAX_SPOT_LIGHTS];
};

struct LightingResult
{
    float3 diffuse;
    float3 specular;
};

// r is how far the light is from the edge of the spot light
// r = (1 - max(cosTheta, cosAlpha)) / (1 - cosAlpha)           
// invSpotAttenuation = 1.0f / max(spotAttenuation, 0.0001f)
// attenuation = 1.f - pow(r, invSpotAttenuation)
float ComputeSpotAttenuation(float3 lightDir, float3 spotDir, float spotAngle, float spotAttenuation)
{
    float cosTheta = dot(lightDir, spotDir);
    float cosAlpha = cos(radians(spotAngle * 0.5f));
    float r = (1 - max(cosTheta, cosAlpha)) / (1 - cosAlpha);

    float invSpotAttenuation = 1.0f / max(spotAttenuation, 0.0001f);

    return 1.f - pow(r, invSpotAttenuation);
}

// attenuation_range = max(1 - (distance / range)^4, 0)
// attenuation = attenuation_range * distance^-attenuation
float ComputeOmniAttenuation(float distance, float range, float attenuation)
{
    float invRange = 1.0f / max(range, 0.0001f);
    float normalizedDistance = distance * invRange;
    normalizedDistance *= normalizedDistance;
    normalizedDistance *= normalizedDistance;
    normalizedDistance = max(1.0 - normalizedDistance, 0.0);
    normalizedDistance *= normalizedDistance;

    return normalizedDistance * pow(max(distance, 0.1), -attenuation);
}

float3 ComputeDiffuse(float3 worldPos, float3 worldNormal)
{
    float3 normal = normalize(worldNormal);
    float3 ambient = float3(0.2f, 0.2f, 0.2f);

    float3 sunLightDir = normalize(-directionalLight.Direction);
    float NdotSunDir = saturate(dot(normal, sunLightDir));
    float3 sunDiffuse = NdotSunDir * directionalLight.Intensity * directionalLight.Color;

    float3 pointAccum = 0.0f;
    for (int pointLightIndex = 0; pointLightIndex < NumPointLights; ++pointLightIndex)
    {
        float3 toLight = PointLights[pointLightIndex].Position - worldPos;
        float dist = length(toLight);
        if (dist <= 0.0001f || dist > PointLights[pointLightIndex].Range)
        {
            continue;
        }

        float3 toLightDir = toLight / dist;
        float NdotLightDir = saturate(dot(normal, toLightDir));

        float atten = ComputeOmniAttenuation(dist, PointLights[pointLightIndex].Range, PointLights[pointLightIndex].Attenuation);

        float3 diffuse = NdotLightDir * PointLights[pointLightIndex].Color * PointLights[pointLightIndex].Intensity;
        pointAccum += diffuse * atten;
    }

    float3 spotAccum = 0.0f;
    for (int spotLightIndex = 0; spotLightIndex < NumSpotLights; ++spotLightIndex)
    {
        float3 toLight = SpotLights[spotLightIndex].Position - worldPos;
        float dist = length(toLight);
        if (dist <= 0.0001f || dist > SpotLights[spotLightIndex].Range)
        {
            continue;
        }

        float3 toLightDir = toLight / dist;
        float NdotLightDir = saturate(dot(normal, toLightDir));

        float atten = ComputeOmniAttenuation(dist, SpotLights[spotLightIndex].Range, SpotLights[spotLightIndex].Attenuation);
        float spotAtten = ComputeSpotAttenuation(-toLightDir, normalize(SpotLights[spotLightIndex].Direction), SpotLights[spotLightIndex].SpotAngle, SpotLights[spotLightIndex].SpotAttenuation);

        float3 diffuse = NdotLightDir * SpotLights[spotLightIndex].Color * SpotLights[spotLightIndex].Intensity;
        spotAccum += diffuse * atten * spotAtten;
    }

    return ambient + sunDiffuse + pointAccum + spotAccum;
}

float3 ComputeSpecular(float3 worldPos, float3 worldNormal, float3 viewDir, float strength, float glossiness)
{
    float3 normal = normalize(worldNormal);
    viewDir = normalize(viewDir);
    float specularPower = exp2(1.f + glossiness * 7.f);

    // We need to judge weather the light is facing the surface or not
    // because when the light is in the backward
    // it is still possible to make sunHalfVector be in the forward
    float3 sunSpecular = 0.f;
    float3 sunLightDir = normalize(-directionalLight.Direction);
    float3 sunHalfVector = normalize(sunLightDir + viewDir);
    float sunNdotL = dot(normal, sunLightDir);
    if (sunNdotL > 0.f)
    {
        sunSpecular =
            directionalLight.Intensity *
            directionalLight.Color *
            strength *
            pow(saturate(dot(normal, sunHalfVector)), specularPower);
    }

    float3 pointAccum = 0.0f;
    for (int pointLightIndex = 0; pointLightIndex < NumPointLights; ++pointLightIndex)
    {
        float3 toLight = PointLights[pointLightIndex].Position - worldPos;
        float dist = length(toLight);
        if (dist <= 0.0001f || dist > PointLights[pointLightIndex].Range)
        {
            continue;
        }

        float3 toLightDir = toLight / dist;
        float3 halfVector = normalize(toLightDir + viewDir);
        float NdotL = dot(normal, toLightDir);

        float atten = ComputeOmniAttenuation(dist, PointLights[pointLightIndex].Range, PointLights[pointLightIndex].Attenuation);

        float3 specular = 0.f;
        if (NdotL > 0.f)
        {
            specular = PointLights[pointLightIndex].Intensity * PointLights[pointLightIndex].Color * strength *
                       pow(saturate(dot(normal, halfVector)), specularPower);
            pointAccum += specular * atten;
        }
    }

    float3 spotAccum = 0.0f;
    for (int spotLightIndex = 0; spotLightIndex < NumSpotLights; ++spotLightIndex)
    {
        float3 toLight = SpotLights[spotLightIndex].Position - worldPos;
        float dist = length(toLight);
        if (dist <= 0.0001f || dist > SpotLights[spotLightIndex].Range)
        {
            continue;
        }

        float3 toLightDir = toLight / dist;
        float3 halfVector = normalize(toLightDir + viewDir);
        float NdotL = dot(normal, toLightDir);

        float atten = ComputeOmniAttenuation(dist, SpotLights[spotLightIndex].Range, SpotLights[spotLightIndex].Attenuation);
        float spotAtten = ComputeSpotAttenuation(-toLightDir, normalize(SpotLights[spotLightIndex].Direction), SpotLights[spotLightIndex].SpotAngle, SpotLights[spotLightIndex].SpotAttenuation);

        float3 specular = 0.f;
        if (NdotL > 0.f)
        {
            specular = SpotLights[spotLightIndex].Intensity * SpotLights[spotLightIndex].Color * strength *
                       pow(saturate(dot(normal, halfVector)), specularPower);
            spotAccum += specular * atten * spotAtten;
        }
    }

    return sunSpecular + pointAccum + spotAccum;
}

LightingResult ComputeLighting(float3 worldPos, float3 worldNormal, float3 viewDir, float strength = 0.f, float glossiness = 0.f)
{
    LightingResult result;
    result.diffuse = ComputeDiffuse(worldPos, worldNormal);
    result.specular = ComputeSpecular(worldPos, worldNormal, viewDir, strength, glossiness);
    return result;
}

#endif


struct VertexInput
{
	float3 modelPosition : POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
	float3 modelTangent : TANGENT;
	float3 modelBitangent : BITANGENT;
	float3 modelNormal : NORMAL;
};

struct VertexToPixel
{
	float4 clipPosition : SV_Position;
	float3 worldPos : WORLDPOS;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
	float4 worldTangent : TANGENT;
	float4 worldBitangent : BITANGENT;
	float4 worldNormal : NORMAL;
};

struct PixelOutput
{
	float4 color : SV_Target0;
	float4 normal : SV_Target1;
};

VertexToPixel VertexMain(VertexInput input)
{
	float4 modelPosition = float4(input.modelPosition, 1);
	float4 worldPosition = mul(ModelToWorldTransform, modelPosition);
	float4 cameraPosition = mul(WorldToCameraTransform, worldPosition);
	float4 renderPosition = mul(CameraToRenderTransform, cameraPosition);
	float4 clipPosition = mul(RenderToClipTransform, renderPosition);

	float4 worldTangent = mul(ModelToWorldTransform, float4(input.modelTangent, 0.0f));
	float4 worldBitangent = mul(ModelToWorldTransform, float4(input.modelBitangent, 0.0f));
	float4 worldNormal = mul(ModelToWorldTransform, float4(input.modelNormal, 0.0f));

	VertexToPixel v2p;
	v2p.clipPosition = clipPosition;
	v2p.worldPos = worldPosition.xyz;
	v2p.color = input.color;
	v2p.uv = input.uv;
	v2p.worldTangent = worldTangent;
	v2p.worldBitangent = worldBitangent;
	v2p.worldNormal = worldNormal;
	return v2p;
}

PixelOutput PixelMain(VertexToPixel input)
{
	PixelOutput output;

	float4 textureColor = DiffuseTexture.Sample(DiffuseSampler, input.uv);
	float4 vertexColor = input.color;
	float4 modelColor = ModelColor;
	float3 cameraWorldPos = mul(CameraToWorldTransform, float4(0.0f, 0.0f, 0.0f, 1.0f)).xyz;
	float3 viewDir = normalize(cameraWorldPos - input.worldPos);
	LightingResult lighting = ComputeLighting(input.worldPos, input.worldNormal.xyz, viewDir, 0.0f, 0.0f);
	float3 albedo = textureColor.xyz * vertexColor.xyz * modelColor.xyz;

	float4 color;
	color.xyz = albedo * lighting.diffuse + lighting.specular;
	color.a = textureColor.a * vertexColor.a * modelColor.a;

	clip(color.a - 0.01f);

	output.color = color;
	output.normal = float4(input.worldNormal.xyz * 0.5f + 0.5f, 1.f);

	return output;
}
)MING_SHADER";

std::string_view const DefaultUI = R"MING_SHADER(#define COMMON_ENABLE_SURFACE_TEXTURES
#ifndef COMMON_BUFFERS_HLSLI
#define COMMON_BUFFERS_HLSLI

cbuffer CameraConstants : register(b2)
{
    float4x4 WorldToCameraTransform;
    float4x4 CameraToRenderTransform;
    float4x4 RenderToClipTransform;
    float4x4 CameraToWorldTransform;
    float4x4 ClipToCameraTransform;
};

cbuffer ModelConstants : register(b3)
{
    float4x4 ModelToWorldTransform;
    float4 ModelColor;
};

cbuffer PostProcessConstants : register(b4)
{
    float2 ScreenSize;
    float CameraNear;
    float CameraFar;
};

cbuffer FrameConstants : register(b5)
{
    float Time;
    float DeltaSeconds;
    float2 FramePadding;
};

#ifdef COMMON_ENABLE_SURFACE_TEXTURES
Texture2D DiffuseTexture : register(t0);
SamplerState DiffuseSampler : register(s0);
#endif

#ifdef COMMON_ENABLE_POSTPROCESS_TEXTURES
Texture2D ColorTexture : register(t0);
Texture2D DepthTexture : register(t1);
Texture2D NormalTexture : register(t2);
SamplerState ColorSampler : register(s0);
SamplerState DepthSampler : register(s1);
SamplerState NormalSampler : register(s2);
#endif

#endif


struct VertexInput
{
	float3 modelPosition : POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
};

struct VertexToPixel
{
	float4 clipPosition : SV_Position;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
};

struct PixelOutput
{
	float4 color : SV_Target0;
};

VertexToPixel VertexMain(VertexInput input)
{
	float4 modelPosition = float4(input.modelPosition, 1);
	float4 clipPosition = mul(RenderToClipTransform, modelPosition);

	VertexToPixel v2p;
	v2p.clipPosition = clipPosition;
	v2p.color = input.color;
	v2p.uv = input.uv;
	return v2p;
}

PixelOutput PixelMain(VertexToPixel input)
{
	PixelOutput output;

	float4 textureColor = DiffuseTexture.Sample(DiffuseSampler, input.uv);
	float4 vertexColor = input.color;
	float4 modelColor = ModelColor;

	float4 color;
	color.xyz = textureColor.xyz * vertexColor.xyz * modelColor.xyz;
	color.a = textureColor.a * vertexColor.a * modelColor.a;

	clip(color.a - 0.01f);

	output.color = color;

	return output;
}
)MING_SHADER";

std::string_view const DefaultUnlit = R"MING_SHADER(#define COMMON_ENABLE_SURFACE_TEXTURES
#ifndef COMMON_BUFFERS_HLSLI
#define COMMON_BUFFERS_HLSLI

cbuffer CameraConstants : register(b2)
{
    float4x4 WorldToCameraTransform;
    float4x4 CameraToRenderTransform;
    float4x4 RenderToClipTransform;
    float4x4 CameraToWorldTransform;
    float4x4 ClipToCameraTransform;
};

cbuffer ModelConstants : register(b3)
{
    float4x4 ModelToWorldTransform;
    float4 ModelColor;
};

cbuffer PostProcessConstants : register(b4)
{
    float2 ScreenSize;
    float CameraNear;
    float CameraFar;
};

cbuffer FrameConstants : register(b5)
{
    float Time;
    float DeltaSeconds;
    float2 FramePadding;
};

#ifdef COMMON_ENABLE_SURFACE_TEXTURES
Texture2D DiffuseTexture : register(t0);
SamplerState DiffuseSampler : register(s0);
#endif

#ifdef COMMON_ENABLE_POSTPROCESS_TEXTURES
Texture2D ColorTexture : register(t0);
Texture2D DepthTexture : register(t1);
Texture2D NormalTexture : register(t2);
SamplerState ColorSampler : register(s0);
SamplerState DepthSampler : register(s1);
SamplerState NormalSampler : register(s2);
#endif

#endif


struct VertexInput
{
	float3 modelPosition : POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
	float3 modelTangent : TANGENT;
	float3 modelBitangent : BITANGENT;
	float3 modelNormal : NORMAL;
};

struct VertexToPixel
{
	float4 clipPosition : SV_Position;
	float3 worldPos : WORLDPOS;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
	float4 worldTangent : TANGENT;
	float4 worldBitangent : BITANGENT;
	float4 worldNormal : NORMAL;
};

struct PixelOutput
{
	float4 color : SV_Target0;
};

VertexToPixel VertexMain(VertexInput input)
{
	float4 modelPosition = float4(input.modelPosition, 1);
	float4 worldPosition = mul(ModelToWorldTransform, modelPosition);
	float4 cameraPosition = mul(WorldToCameraTransform, worldPosition);
	float4 renderPosition = mul(CameraToRenderTransform, cameraPosition);
	float4 clipPosition = mul(RenderToClipTransform, renderPosition);

	float4 worldTangent = mul(ModelToWorldTransform, float4(input.modelTangent, 0.0f));
	float4 worldBitangent = mul(ModelToWorldTransform, float4(input.modelBitangent, 0.0f));
	float4 worldNormal = mul(ModelToWorldTransform, float4(input.modelNormal, 0.0f));

	VertexToPixel v2p;
	v2p.clipPosition = clipPosition;
	v2p.worldPos = worldPosition.xyz;
	v2p.color = input.color;
	v2p.uv = input.uv;
	v2p.worldTangent = worldTangent;
	v2p.worldBitangent = worldBitangent;
	v2p.worldNormal = worldNormal;
	return v2p;
}

PixelOutput PixelMain(VertexToPixel input)
{
	PixelOutput output;

	float4 textureColor = DiffuseTexture.Sample(DiffuseSampler, input.uv);
	float4 vertexColor = input.color;
	float4 modelColor = ModelColor;

	float4 color;
	color.xyz = textureColor.xyz * vertexColor.xyz * modelColor.xyz;
	color.a = textureColor.a * vertexColor.a * modelColor.a;

	clip(color.a - 0.01f);

	output.color = color;

	return output;
}
)MING_SHADER";

std::string_view const GizmosAxis = R"MING_SHADER(#define COMMON_ENABLE_SURFACE_TEXTURES
#ifndef COMMON_BUFFERS_HLSLI
#define COMMON_BUFFERS_HLSLI

cbuffer CameraConstants : register(b2)
{
    float4x4 WorldToCameraTransform;
    float4x4 CameraToRenderTransform;
    float4x4 RenderToClipTransform;
    float4x4 CameraToWorldTransform;
    float4x4 ClipToCameraTransform;
};

cbuffer ModelConstants : register(b3)
{
    float4x4 ModelToWorldTransform;
    float4 ModelColor;
};

cbuffer PostProcessConstants : register(b4)
{
    float2 ScreenSize;
    float CameraNear;
    float CameraFar;
};

cbuffer FrameConstants : register(b5)
{
    float Time;
    float DeltaSeconds;
    float2 FramePadding;
};

#ifdef COMMON_ENABLE_SURFACE_TEXTURES
Texture2D DiffuseTexture : register(t0);
SamplerState DiffuseSampler : register(s0);
#endif

#ifdef COMMON_ENABLE_POSTPROCESS_TEXTURES
Texture2D ColorTexture : register(t0);
Texture2D DepthTexture : register(t1);
Texture2D NormalTexture : register(t2);
SamplerState ColorSampler : register(s0);
SamplerState DepthSampler : register(s1);
SamplerState NormalSampler : register(s2);
#endif

#endif


struct VertexInput
{
	float3 modelPosition : POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
};

struct VertexToPixel
{
	float4 clipPosition : SV_Position;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
};

struct PixelOutput
{
	float4 color : SV_Target0;
};

VertexToPixel VertexMain(VertexInput input)
{
	VertexToPixel v2p;

	float3 positionA = mul(ModelToWorldTransform, float4(0.f, 0.f, 0.f, 1.0f)).xyz;
	float3 positionB = float3(ModelToWorldTransform[0][0], ModelToWorldTransform[1][1], ModelToWorldTransform[2][2]);

	float4 clipPositionA = mul(RenderToClipTransform, mul(CameraToRenderTransform, mul(WorldToCameraTransform, float4(positionA, 1.0f))));
	float4 clipPositionB = mul(RenderToClipTransform, mul(CameraToRenderTransform, mul(WorldToCameraTransform, float4(positionB, 1.0f))));

	float2 screenPosA = ScreenSize * 0.5f * (clipPositionA.xy / clipPositionA.w + 1.0f);
	float2 screenPosB = ScreenSize * 0.5f * (clipPositionB.xy / clipPositionB.w + 1.0f);

	float width = 2.f;
	float2 xBasis = normalize(screenPosB - screenPosA);
	float2 yBasis = float2(-xBasis.y, xBasis.x);
	
	screenPosA += (input.modelPosition.x * xBasis + yBasis * input.modelPosition.y) * width;
	screenPosB += (input.modelPosition.x * xBasis + yBasis * input.modelPosition.y) * width;
	
	float2 screenPos = lerp(screenPosA, screenPosB, input.modelPosition.z);
	float4 clipPosition = lerp(clipPositionA, clipPositionB, input.modelPosition.z);

	v2p.clipPosition = float4(((screenPos / ScreenSize) * 2.0f - 1.0f) * clipPosition.w, clipPosition.z, clipPosition.w);
	v2p.color = input.color;
	v2p.uv = input.uv;

	return v2p;
}

PixelOutput PixelMain(VertexToPixel input)
{
	PixelOutput output;

	output.color = input.color;

	return output;
}
)MING_SHADER";

std::string_view const GizmosGrid = R"MING_SHADER(#define COMMON_ENABLE_SURFACE_TEXTURES
#ifndef COMMON_BUFFERS_HLSLI
#define COMMON_BUFFERS_HLSLI

cbuffer CameraConstants : register(b2)
{
    float4x4 WorldToCameraTransform;
    float4x4 CameraToRenderTransform;
    float4x4 RenderToClipTransform;
    float4x4 CameraToWorldTransform;
    float4x4 ClipToCameraTransform;
};

cbuffer ModelConstants : register(b3)
{
    float4x4 ModelToWorldTransform;
    float4 ModelColor;
};

cbuffer PostProcessConstants : register(b4)
{
    float2 ScreenSize;
    float CameraNear;
    float CameraFar;
};

cbuffer FrameConstants : register(b5)
{
    float Time;
    float DeltaSeconds;
    float2 FramePadding;
};

#ifdef COMMON_ENABLE_SURFACE_TEXTURES
Texture2D DiffuseTexture : register(t0);
SamplerState DiffuseSampler : register(s0);
#endif

#ifdef COMMON_ENABLE_POSTPROCESS_TEXTURES
Texture2D ColorTexture : register(t0);
Texture2D DepthTexture : register(t1);
Texture2D NormalTexture : register(t2);
SamplerState ColorSampler : register(s0);
SamplerState DepthSampler : register(s1);
SamplerState NormalSampler : register(s2);
#endif

#endif


struct VertexInput
{
	float3 modelPosition : POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
	float3 modelTangent : TANGENT;
	float3 modelBitangent : BITANGENT;
	float3 modelNormal : NORMAL;
};

struct VertexToPixel
{
	float4 clipPosition : SV_Position;
	float3 worldPos : WORLDPOS;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
	float4 worldTangent : TANGENT;
	float4 worldBitangent : BITANGENT;
	float4 worldNormal : NORMAL;
};

struct PixelOutput
{
	float4 color : SV_Target0;
};

VertexToPixel VertexMain(VertexInput input)
{
	float4 modelPosition = float4(input.modelPosition, 1);
	float4 worldPosition = mul(ModelToWorldTransform, modelPosition);
	float4 cameraPosition = mul(WorldToCameraTransform, worldPosition);
	float4 renderPosition = mul(CameraToRenderTransform, cameraPosition);
	float4 clipPosition = mul(RenderToClipTransform, renderPosition);

	float4 worldTangent = mul(ModelToWorldTransform, float4(input.modelTangent, 0.0f));
	float4 worldBitangent = mul(ModelToWorldTransform, float4(input.modelBitangent, 0.0f));
	float4 worldNormal = mul(ModelToWorldTransform, float4(input.modelNormal, 0.0f));

	VertexToPixel v2p;
	v2p.clipPosition = clipPosition;
	v2p.worldPos = worldPosition.xyz;
	v2p.color = input.color;
	v2p.uv = input.uv;
	v2p.worldTangent = worldTangent;
	v2p.worldBitangent = worldBitangent;
	v2p.worldNormal = worldNormal;
	return v2p;
}

PixelOutput PixelMain(VertexToPixel input)
{
	PixelOutput output;

	float4 textureColor = DiffuseTexture.Sample(DiffuseSampler, input.uv);
	float4 vertexColor = input.color;
	float4 modelColor = ModelColor;

	float3 cameraPosition = mul(CameraToWorldTransform, float4(0, 0, 0, 1)).xyz;
	float3 cameraToPixel = normalize(input.worldPos - cameraPosition);

	float angleFade = abs(dot(float3(0.f, 0.f, 1.f), cameraToPixel));
	angleFade = smoothstep(0.1f, 0.4f, angleFade);

	float distFade = 1.f - distance(input.worldPos, float3(cameraPosition.xy, 0.f)) / 50.f;
	distFade = smoothstep(0.02f, 0.3f, distFade);

	float alpha = 1.f * angleFade * distFade;

	float4 color;
	color.xyz = textureColor.xyz * vertexColor.xyz * modelColor.xyz;
	color.a = alpha;

	clip(color.a - 0.01f);

	output.color = color;

	return output;
}
)MING_SHADER";

std::string_view const PostProcessCopy = R"MING_SHADER(#define COMMON_ENABLE_SURFACE_TEXTURES
#ifndef COMMON_BUFFERS_HLSLI
#define COMMON_BUFFERS_HLSLI

cbuffer CameraConstants : register(b2)
{
    float4x4 WorldToCameraTransform;
    float4x4 CameraToRenderTransform;
    float4x4 RenderToClipTransform;
    float4x4 CameraToWorldTransform;
    float4x4 ClipToCameraTransform;
};

cbuffer ModelConstants : register(b3)
{
    float4x4 ModelToWorldTransform;
    float4 ModelColor;
};

cbuffer PostProcessConstants : register(b4)
{
    float2 ScreenSize;
    float CameraNear;
    float CameraFar;
};

cbuffer FrameConstants : register(b5)
{
    float Time;
    float DeltaSeconds;
    float2 FramePadding;
};

#ifdef COMMON_ENABLE_SURFACE_TEXTURES
Texture2D DiffuseTexture : register(t0);
SamplerState DiffuseSampler : register(s0);
#endif

#ifdef COMMON_ENABLE_POSTPROCESS_TEXTURES
Texture2D ColorTexture : register(t0);
Texture2D DepthTexture : register(t1);
Texture2D NormalTexture : register(t2);
SamplerState ColorSampler : register(s0);
SamplerState DepthSampler : register(s1);
SamplerState NormalSampler : register(s2);
#endif

#endif

#ifndef COMMON_MATH_HLSLI
#define COMMON_MATH_HLSLI

float LinearizeDepth(float depth, float zNear, float zFar)
{
    return (zNear * zFar) / (zFar - depth * (zFar - zNear));
}

float3 DecodeNormal(float3 normal)
{
    float3 result = normal * 2.0f - 1.0f;
    if (length(result) < 0.5f)
    {
        result = float3(0.0f, 0.0f, 0.0f);
    }
    else
    {
        result = normalize(result);
    }

    return result;
}

float2 ScreenUVToClipXY(float2 screenUV)
{
    return float2(screenUV.x * 2.0f - 1.0f, 1.0f - screenUV.y * 2.0f);
}

float3 ReconstructWorldPosition(float2 uv, float depth, float4x4 clipToCamera, float4x4 cameraToWorld)
{
    float4 clipPos = float4(ScreenUVToClipXY(uv), depth, 1.0f);
    float4 cameraPos = mul(clipToCamera, clipPos);
    float4 worldPos = mul(cameraToWorld, cameraPos);
    return worldPos.xyz / worldPos.w;
}

#endif


struct VS_INPUT
{
    float3 localPosition : POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD0;
    float3 modelTangent : TANGENT;
    float3 modelBitangent : BITANGENT;
    float3 modelNormal : NORMAL;
};

struct VS_OUTPUT
{
    float4 position : SV_Position;
    float4 color : COLOR;
    float2 uv : TEXCOORD0;
};

VS_OUTPUT VertexMain(VS_INPUT input)
{
    float4 localPosition = float4(input.localPosition, 1);

    VS_OUTPUT o;
    o.position = localPosition;
    o.color = input.color;
    o.uv = input.uv;
    return o;
}

float4 PixelMain(VS_OUTPUT input) : SV_Target0
{
    float4 textureColor = DiffuseTexture.Sample(DiffuseSampler, input.uv);

    return textureColor;
}
)MING_SHADER";

std::string_view const TransformGizmosArc = R"MING_SHADER(#define COMMON_ENABLE_SURFACE_TEXTURES
#ifndef COMMON_BUFFERS_HLSLI
#define COMMON_BUFFERS_HLSLI

cbuffer CameraConstants : register(b2)
{
    float4x4 WorldToCameraTransform;
    float4x4 CameraToRenderTransform;
    float4x4 RenderToClipTransform;
    float4x4 CameraToWorldTransform;
    float4x4 ClipToCameraTransform;
};

cbuffer ModelConstants : register(b3)
{
    float4x4 ModelToWorldTransform;
    float4 ModelColor;
};

cbuffer PostProcessConstants : register(b4)
{
    float2 ScreenSize;
    float CameraNear;
    float CameraFar;
};

cbuffer FrameConstants : register(b5)
{
    float Time;
    float DeltaSeconds;
    float2 FramePadding;
};

#ifdef COMMON_ENABLE_SURFACE_TEXTURES
Texture2D DiffuseTexture : register(t0);
SamplerState DiffuseSampler : register(s0);
#endif

#ifdef COMMON_ENABLE_POSTPROCESS_TEXTURES
Texture2D ColorTexture : register(t0);
Texture2D DepthTexture : register(t1);
Texture2D NormalTexture : register(t2);
SamplerState ColorSampler : register(s0);
SamplerState DepthSampler : register(s1);
SamplerState NormalSampler : register(s2);
#endif

#endif


struct VertexInput
{
	float3 modelPosition : POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
	float3 modelTangent : TANGENT;
	float3 modelBitangent : BITANGENT;
	float3 modelNormal : NORMAL;
};

struct VertexToPixel
{
	float4 clipPosition : SV_Position;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
};

struct PixelOutput
{
	float4 color : SV_Target0;
};

VertexToPixel VertexMain(VertexInput input)
{
	float4 modelDirection = float4(input.modelPosition, 0);
	float4 worldDirection = mul(ModelToWorldTransform, modelDirection);
	float4 cameraDirection = mul(WorldToCameraTransform, worldDirection);
	cameraDirection.xyz = normalize(cameraDirection.xyz);

	float orientation = dot(cameraDirection, float4(-1.f, 0, 0, 0));
	if (orientation >= -0.005f)
	{
		input.modelPosition += input.modelNormal * 0.01f;
	}

	float4 modelPosition = float4(input.modelPosition, 1);
	float4 worldPosition = mul(ModelToWorldTransform, modelPosition);
	float4 cameraPosition = mul(WorldToCameraTransform, worldPosition);
	float4 renderPosition = mul(CameraToRenderTransform, cameraPosition);
	float4 clipPosition = mul(RenderToClipTransform, renderPosition);

	VertexToPixel v2p;
	v2p.clipPosition = clipPosition;
	v2p.color = input.color;
	v2p.uv = input.uv;

	return v2p;
}

PixelOutput PixelMain(VertexToPixel input)
{
	PixelOutput output;

	float4 textureColor = DiffuseTexture.Sample(DiffuseSampler, input.uv);
	float4 vertexColor = input.color;
	float4 modelColor = ModelColor;

	float4 color;
	color.xyz = textureColor.xyz * vertexColor.xyz * modelColor.xyz;
	color.a = vertexColor.a;

	clip(color.a - 0.01f);

	output.color = color;

	return output;
}
)MING_SHADER";
} // namespace BuiltinShaders
