#include "Engine/Renderer/DebugRenderer.hpp"

#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/Engine.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Math/MathUtils.hpp"

namespace
{
    enum class DebugObjectType
    {
        WORLD_SPHERE,
        WORLD_WIRE_SPHERE,
        WORLD_CYLINDER,
        WORLD_WIRE_CYLINDER,
        WORLD_ARROW,
        WORLD_WIRE_ARROW,
        WORLD_TEXT,
        WORLD_BILLBOARD_TEXT,
        WORLD_GRID,
        SCREEN_TEXT,
        MESSAGE
    };

    struct DebugObject
    {
        DebugObjectType type;

        DebugRenderMode mode = DebugRenderMode::USE_DEPTH;

        float totalDuration = 0.f;
        float remainingDuration = 0.f;

        Rgba8 startColor = Rgba8::WHITE;
        Rgba8 endColor = Rgba8::WHITE;

        // geometry
        Vec3 start = Vec3::ZERO;
        Vec3 end = Vec3::ZERO;
        Vec3 center = Vec3::ZERO;
        float radius = 0.f;

        // transform
        Matrix4x4 transform;

        // text
        std::string text;
        float textHeight = 0.f;
        Vec2 alignment = Vec2(0.5f, 0.5f);

        // screen
        AABB2 screenBox;

        // grid (hard-coded in DebugAddWorldGrid; stored here so renderer doesn't need to regenerate)
        std::vector<Vertex_PCU> verts;
    };

    static DebugRenderConfig s_debugRenderConfig;
    static std::vector<DebugObject> s_debugObjects;
    static std::vector<DebugObject> s_debugMessages;
    static bool s_isVisible = true;

    DebugObject MakeDebugObject(DebugObjectType type, float duration,
        Rgba8 const& startColor, Rgba8 const& endColor,
        DebugRenderMode mode = DebugRenderMode::USE_DEPTH)
    {
        DebugObject object;
        object.type = type;
        object.mode = mode;
        object.totalDuration = duration;
        object.remainingDuration = duration;
        object.startColor = startColor;
        object.endColor = endColor;
        return object;
    }

    Rgba8 GetDebugObjectColor(DebugObject const& obj)
    {
        if (obj.totalDuration < 0.f)
        {
            return obj.startColor;
        }

        if (obj.totalDuration == 0.f)
        {
            return obj.startColor;
        }

        float t = 1.f - (obj.remainingDuration / obj.totalDuration);
        t = GetClamped(t, 0.f, 1.f);
        return Interpolate(obj.startColor, obj.endColor, t);
    }

    void ApplyDebugRenderMode(Renderer* renderer, DebugObject const& obj)
    {
        if (renderer == nullptr)
        {
            return;
        }

        if (obj.type == DebugObjectType::SCREEN_TEXT || obj.type == DebugObjectType::MESSAGE)
        {
            renderer->SetBlendMode(BlendMode::ALPHA);
            renderer->SetDepthMode(DepthMode::READ_ONLY_ALWAYS);
            renderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
            return;
        }
        else
        {
            renderer->SetBlendMode(BlendMode::ALPHA);

            switch (obj.mode)
            {
            case DebugRenderMode::ALWAYS:
                renderer->SetDepthMode(DepthMode::DISABLED);
                break;

            case DebugRenderMode::USE_DEPTH:
                renderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
                break;

            case DebugRenderMode::X_RAY:
                renderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
                break;

            default:
                renderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
                break;
            }

            switch (obj.type)
            {
            case DebugObjectType::WORLD_WIRE_SPHERE:
            case DebugObjectType::WORLD_WIRE_CYLINDER:
            case DebugObjectType::WORLD_WIRE_ARROW:
                renderer->SetRasterizerMode(RasterizerMode::WIREFRAME_CULL_NONE);
                break;

            default:
                renderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
                break;
            }
        }
        
    }

    void DrawWorldObject(Renderer* renderer, BitmapFont* font, Camera const& camera, DebugObject const& obj)
    {
        std::vector<Vertex_PCU> verts;
        verts.reserve(2048);

        Rgba8 color = GetDebugObjectColor(obj);

        switch (obj.type)
        {
        case DebugObjectType::WORLD_SPHERE:
            AddVertsForSphere3D(verts, obj.center, obj.radius, color);
            renderer->BindTexture(nullptr);
            renderer->SetModelConstants(Matrix4x4::IDENTITY, Rgba8::WHITE);
            renderer->DrawVertexArray(verts);
            break;

        case DebugObjectType::WORLD_WIRE_SPHERE:
            AddVertsForSphere3D(verts, obj.center, obj.radius, color);
            renderer->BindTexture(nullptr);
            renderer->SetModelConstants(Matrix4x4::IDENTITY, Rgba8::WHITE);
            renderer->DrawVertexArray(verts);
            break;

        case DebugObjectType::WORLD_CYLINDER:
            AddVertsForCylinder3D(verts, obj.start, obj.end, obj.radius, color);
            renderer->BindTexture(nullptr);
            renderer->SetModelConstants(Matrix4x4::IDENTITY, Rgba8::WHITE);
            renderer->DrawVertexArray(verts);
            break;

        case DebugObjectType::WORLD_WIRE_CYLINDER:
            AddVertsForCylinder3D(verts, obj.start, obj.end, obj.radius, color);
            renderer->BindTexture(nullptr);
            renderer->SetModelConstants(Matrix4x4::IDENTITY, Rgba8::WHITE);
            renderer->DrawVertexArray(verts);
            break;

        case DebugObjectType::WORLD_ARROW:
            AddVertsForArrow3D(verts, obj.start, obj.end, obj.radius, color);
            renderer->BindTexture(nullptr);
            renderer->SetModelConstants(Matrix4x4::IDENTITY, Rgba8::WHITE);
            renderer->DrawVertexArray(verts);
            break;

        case DebugObjectType::WORLD_WIRE_ARROW:
            AddVertsForArrow3D(verts, obj.start, obj.end, obj.radius, color);
            renderer->BindTexture(nullptr);
            renderer->SetModelConstants(Matrix4x4::IDENTITY, Rgba8::WHITE);
            renderer->DrawVertexArray(verts);
            break;

        case DebugObjectType::WORLD_TEXT:
        {
            if (font == nullptr)
            {
                break;
            }

            font->AddVertsForText3DAtOriginXForward(verts, obj.textHeight, obj.text, color, 1.0f, obj.alignment);

            // Put text in world using model matrix
            TransformVertexArray3D(verts, obj.transform);

            renderer->BindTexture(&font->GetTexture());
            renderer->SetModelConstants(Matrix4x4::IDENTITY, Rgba8::WHITE);
            renderer->DrawVertexArray(verts);
        }
        break;

        case DebugObjectType::WORLD_BILLBOARD_TEXT:
        {
            if (font == nullptr)
            {
                break;
            }

            font->AddVertsForText3DAtOriginXForward(verts, obj.textHeight, obj.text, color, 1.0f, obj.alignment);

            Matrix4x4 billboard = GetBillboardTransform(BillboardType::FULL_OPPOSING, camera.GetCameraToWorldTransform(), obj.center);
            TransformVertexArray3D(verts, billboard);
            

            renderer->BindTexture(&font->GetTexture());
            renderer->SetModelConstants(Matrix4x4::IDENTITY, Rgba8::WHITE);
            renderer->DrawVertexArray(verts);
        }
        break;

        case DebugObjectType::WORLD_GRID:
        {
            if (obj.verts.empty())
            {
                break;
            }

            renderer->BindTexture(nullptr);
            renderer->SetModelConstants(Matrix4x4::IDENTITY, Rgba8::WHITE);
            renderer->DrawVertexArray(obj.verts);
        }
        break;

        default:
            break;
        }
    }

    void DrawScreenObject(Renderer* renderer, BitmapFont* font, Camera const& camera, DebugObject const& obj, int lineNum = -1)
    {
        if (renderer == nullptr || font == nullptr)
        {
            return;
        }

        if (obj.type != DebugObjectType::SCREEN_TEXT && obj.type != DebugObjectType::MESSAGE)
        {
            return;
        }

        Rgba8 color = GetDebugObjectColor(obj);

        AABB2 box = obj.screenBox;
        if (obj.type == DebugObjectType::MESSAGE)
        {
            Vec2 cameraDimensions = camera.GetOrthographicBounds().GetDimensions();
            float cellHeight = obj.textHeight > 0.f ? obj.textHeight : 20.f;
            float linePadding = 2.f;
            float top = cameraDimensions.y - 10.f - (cellHeight + linePadding) * (float)lineNum;
            box = AABB2(Vec2(10.f, top - cellHeight), Vec2(cameraDimensions.x - 10.f, top));
        }

        std::vector<Vertex_PCU> verts;
        verts.reserve(1024);

        float cellHeight = obj.textHeight > 0.f ? obj.textHeight : 20.f;
        font->AddVertsForTextInBox2D(verts, obj.text, box, cellHeight, color, 0.85f, obj.alignment, TextBoxMode::SHRINK_TO_FIT);

        renderer->BindTexture(&font->GetTexture());
        renderer->SetModelConstants(Matrix4x4::IDENTITY, Rgba8::WHITE);
        renderer->DrawVertexArray(verts);
    }

    void UpdateDebugObjectLifetimes(std::vector<DebugObject>& objects, float deltaSeconds)
    {
        for (int i = (int)objects.size() - 1; i >= 0; --i)
        {
            DebugObject& obj = objects[i];

            if (obj.totalDuration < 0.f)
            {
                continue;
            }

            obj.remainingDuration -= deltaSeconds;

            if (obj.remainingDuration <= 0.f)
            {
                objects.erase(objects.begin() + i);
            }
        }
    }

    void ResetRendererStates(Renderer* renderer)
    {
        if (renderer == nullptr)
        {
            return;
        }
        renderer->SetBlendMode(BlendMode::ALPHA);
        renderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
        renderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
    }
}

// Setup
void DebugRenderSystemStartup(const DebugRenderConfig& config)
{
    s_debugRenderConfig = config;
    s_debugObjects.clear();
    s_debugMessages.clear();
    s_isVisible = true;

    g_engine->m_eventSystem->SubscribeEventCallbackFunction("Dev_DebugRenderClear", Command_DebugRenderClear);
    g_engine->m_eventSystem->SubscribeEventCallbackFunction("Dev_DebugRenderToggle", Command_DebugRenderToggle);
}

void DebugRenderSystemShutdown()
{
    g_engine->m_eventSystem->UnsubscribeEventCallbackFunction("Dev_DebugRenderClear", Command_DebugRenderClear);
    g_engine->m_eventSystem->UnsubscribeEventCallbackFunction("Dev_DebugRenderToggle", Command_DebugRenderToggle);

    s_debugRenderConfig.m_renderer = nullptr;
    s_debugObjects.clear();
    s_debugMessages.clear();
    s_isVisible = false;
}

// Control
void DebugRenderSetVisible()
{
    s_isVisible = true;
}

void DebugRenderSetHidden()
{
    s_isVisible = false;
}

void DebugRenderClear()
{
    s_debugObjects.clear();
    s_debugMessages.clear();
}

// Geometry
void DebugAddWorldSphere(const Vec3& center, float radius, float duration,
    const Rgba8& startColor, const Rgba8& endColor, DebugRenderMode mode)
{
    DebugObject object = MakeDebugObject(DebugObjectType::WORLD_SPHERE, duration, startColor, endColor, mode);
    object.center = center;
    object.radius = radius;
    s_debugObjects.push_back(object);
}

void DebugAddWorldWireSphere(const Vec3& center, float radius, float duration,
    const Rgba8& startColor, const Rgba8& endColor, DebugRenderMode mode)
{
    DebugObject object = MakeDebugObject(DebugObjectType::WORLD_WIRE_SPHERE, duration, startColor, endColor, mode);
    object.center = center;
    object.radius = radius;
    s_debugObjects.push_back(object);
}

void DebugAddWorldCylinder(const Vec3& start, const Vec3& end, float radius, float duration,
    const Rgba8& startColor, const Rgba8& endColor, DebugRenderMode mode)
{
    DebugObject object = MakeDebugObject(DebugObjectType::WORLD_CYLINDER, duration, startColor, endColor, mode);
    object.start = start;
    object.end = end;
    object.radius = radius;
    s_debugObjects.push_back(object);
}

void DebugAddWorldWireCylinder(const Vec3& start, const Vec3& end, float radius, float duration,
    const Rgba8& startColor, const Rgba8& endColor, DebugRenderMode mode)
{
    DebugObject object = MakeDebugObject(DebugObjectType::WORLD_WIRE_CYLINDER, duration, startColor, endColor, mode);
    object.start = start;
    object.end = end;
    object.radius = radius;
    s_debugObjects.push_back(object);
}

void DebugAddWorldArrow(const Vec3& start, const Vec3& end, float radius, float duration,
    const Rgba8& startColor, const Rgba8& endColor, DebugRenderMode mode)
{
    DebugObject object = MakeDebugObject(DebugObjectType::WORLD_ARROW, duration, startColor, endColor, mode);
    object.start = start;
    object.end = end;
    object.radius = radius;
    s_debugObjects.push_back(object);
}

void DebugAddWorldWireArrow(const Vec3& start, const Vec3& end, float radius, float duration,
    const Rgba8& startColor, const Rgba8& endColor, DebugRenderMode mode)
{
    DebugObject object = MakeDebugObject(DebugObjectType::WORLD_WIRE_ARROW, duration, startColor, endColor, mode);
    object.start = start;
    object.end = end;
    object.radius = radius;
    s_debugObjects.push_back(object);
}

void DebugAddBasis(const Matrix4x4& transform, float duration, float length, float radius,
    float colorScale, float alphaScale, DebugRenderMode mode)
{
    Vec3 origin = transform.GetTranslation3D();
    Vec3 xEnd = transform.TransformPosition3D(Vec3(length, 0.f, 0.f));
    Vec3 yEnd = transform.TransformPosition3D(Vec3(0.f, length, 0.f));
    Vec3 zEnd = transform.TransformPosition3D(Vec3(0.f, 0.f, length));

    Rgba8 xColor = Interpolate(Rgba8::BLACK, Rgba8::RED, colorScale);
    Rgba8 yColor = Interpolate(Rgba8::BLACK, Rgba8::GREEN, colorScale);
    Rgba8 zColor = Interpolate(Rgba8::BLACK, Rgba8::BLUE, colorScale);

    xColor.a = (unsigned char)GetClamped((float)xColor.a * alphaScale, 0.f, 255.f);
    yColor.a = (unsigned char)GetClamped((float)yColor.a * alphaScale, 0.f, 255.f);
    zColor.a = (unsigned char)GetClamped((float)zColor.a * alphaScale, 0.f, 255.f);

    DebugAddWorldArrow(origin, xEnd, radius, duration, xColor, xColor, mode);
    DebugAddWorldArrow(origin, yEnd, radius, duration, yColor, yColor, mode);
    DebugAddWorldArrow(origin, zEnd, radius, duration, zColor, zColor, mode);
}

void DebugAddWorldBasis(const Matrix4x4& transform, float duration, DebugRenderMode mode)
{
    DebugAddBasis(transform, duration, 1.0f, 0.1f, 1.0f, 1.0f, mode);
}

void DebugAddWorldText(const std::string& text, const Matrix4x4& transform, float textHeight,
    const Vec2& alignment, float duration,
    const Rgba8& startColor, const Rgba8& endColor, DebugRenderMode mode)
{
    DebugObject object = MakeDebugObject(DebugObjectType::WORLD_TEXT, duration, startColor, endColor, mode);
    object.text = text;
    object.transform = transform;
    object.textHeight = textHeight;
    object.alignment = alignment;
    s_debugObjects.push_back(object);
}

void DebugAddWorldBillboardText(const std::string& text, const Vec3& origin, float textHeight,
    const Vec2& alignment, float duration,
    const Rgba8& startColor, const Rgba8& endColor, DebugRenderMode mode)
{
    DebugObject object = MakeDebugObject(DebugObjectType::WORLD_BILLBOARD_TEXT, duration, startColor, endColor, mode);
    object.text = text;
    object.center = origin;
    object.textHeight = textHeight;
    object.alignment = alignment;
    s_debugObjects.push_back(object);
}

void DebugAddScreenText(const std::string& text, const AABB2& box, float cellHeight, const Vec2& alignment, float duration, const Rgba8& startColor /*= Rgba8::WHITE*/, const Rgba8& endColor /*= Rgba8::WHITE*/)
{
    DebugObject object = MakeDebugObject(DebugObjectType::SCREEN_TEXT, duration, startColor, endColor, DebugRenderMode::ALWAYS);
    object.text = text;
    object.screenBox = box;
    object.textHeight = cellHeight;
    object.alignment = alignment;
    s_debugObjects.push_back(object);
}

void DebugAddMessage(const std::string& text, float duration, const Rgba8& startColor /*= Rgba8::WHITE*/, const Rgba8& endColor /*= Rgba8::WHITE*/)
{
    DebugObject object = MakeDebugObject(DebugObjectType::MESSAGE, duration, startColor, endColor, DebugRenderMode::ALWAYS);
    object.text = text;
    object.textHeight = 20.f;
    object.alignment = Vec2(0.f, 0.5f);
    s_debugMessages.push_back(object);
}

void DebugAddWorldGrid()
{
    // Params are intentionally hard-coded per request.
    constexpr int kHalfExtent = 50;
    constexpr float kLineHeight = 0.01f;

    // Thickness: only Base + Axis (Major treated same as Base)
    constexpr float kBaseThickness = 0.005f;
    constexpr float kAxisThickness = 0.02f;

    // Brightness: only Base + Axis (Major treated same as Base)
    constexpr float kBaseBrightness = 0.25f;
    constexpr float kAxisBrightness = 1.f;

    // Fade settings (hard-coded)
    constexpr float kFadeStart = 10.f;
    constexpr float kFadeEnd = 30.f;

    // Segment size (hard-coded): smaller => smoother fade, more verts
    constexpr float kSegmentLength = 1.f;

    DebugObject object = MakeDebugObject(DebugObjectType::WORLD_GRID, -1.f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::USE_DEPTH);
    object.verts.clear();
    object.verts.reserve((kHalfExtent * 2 + 1) * 2 * 36);

    auto ComputeAlphaForDistance = [](float d) -> unsigned char
    {
        float a = RangeMapClamped(d, kFadeStart, kFadeEnd, 255.f, 0.f);
        return (unsigned char)GetClamped(a, 0.f, 255.f);
    };

    auto AddSegmentAABB = [&object, &ComputeAlphaForDistance](AABB3 const& aabb, Rgba8 baseColor)
    {
        // Use segment center distance to compute alpha
        Vec3 center = aabb.GetCenter();
        float dist = sqrtf(center.x * center.x + center.y * center.y);
        baseColor.a = ComputeAlphaForDistance(dist);
        AddVertsForAABB3D(object.verts, aabb, baseColor);
    };

    for (int lineIndex = -kHalfExtent; lineIndex <= kHalfExtent; ++lineIndex)
    {
        bool const isAxis = (lineIndex == 0);

        float lineThickness = kBaseThickness;
        float brightness = kBaseBrightness;

        if (isAxis)
        {
            lineThickness = kAxisThickness;
            brightness = kAxisBrightness;
        }

        float const lineOffset = static_cast<float>(lineIndex);
        float const halfThickness = lineThickness * 0.5f;

        Rgba8 xParallelColor = Rgba8::RED * brightness;
        Rgba8 yParallelColor = Rgba8::GREEN * brightness;

        if (!isAxis)
        {
            xParallelColor = Rgba8::GRAY;
            yParallelColor = Rgba8::GRAY;
        }

        // Split each long strip into small segments along its length.
        float const minCoord = -static_cast<float>(kHalfExtent);
        float const maxCoord = static_cast<float>(kHalfExtent);

        // X-axis parallel lines (vary Y, span X)
        for (float x = minCoord; x < maxCoord; x += kSegmentLength)
        {
            float x0 = x;
            float x1 = x + kSegmentLength;
            if (x1 > maxCoord)
            {
                x1 = maxCoord;
            }

            AddSegmentAABB(
                AABB3(
                    Vec3(x0, lineOffset - halfThickness, 0.f),
                    Vec3(x1, lineOffset + halfThickness, kLineHeight)),
                xParallelColor);
        }

        // Y-axis parallel lines (vary X, span Y)
        for (float y = minCoord; y < maxCoord; y += kSegmentLength)
        {
            float y0 = y;
            float y1 = y + kSegmentLength;
            if (y1 > maxCoord)
            {
                y1 = maxCoord;
            }

            AddSegmentAABB(
                AABB3(
                    Vec3(lineOffset - halfThickness, y0, 0.f),
                    Vec3(lineOffset + halfThickness, y1, kLineHeight)),
                yParallelColor);
        }
    }

    s_debugObjects.push_back(std::move(object));
}

// Output
void DebugRenderBeginFrame()
{
}

void DebugRenderWorld(const Camera& camera)
{
    if (!s_isVisible)
    {
        return;
    }

    Renderer* renderer = s_debugRenderConfig.m_renderer;
    if (renderer == nullptr)
    {
        return;
    }

    BitmapFont* font = renderer->CreateOrGetBitmapFont(
        Stringf("%s%s", s_debugRenderConfig.m_fontPath.c_str(), s_debugRenderConfig.m_fontName.c_str()).c_str()
    );

    // First pass: X_RAY objects only, with modified alpha for see-through effect.
    float const xrayAlphaMultiplier = 0.2f;
    for (DebugObject const& obj : s_debugObjects)
    {
        if (obj.type == DebugObjectType::SCREEN_TEXT || obj.type == DebugObjectType::MESSAGE)
        {
            continue;
        }

        if (obj.mode != DebugRenderMode::X_RAY)
        {
            continue;
        }

        DebugObject xrayObject = obj;
        xrayObject.mode = DebugRenderMode::ALWAYS;
        xrayObject.startColor.a = (unsigned char)((float)xrayObject.startColor.a * xrayAlphaMultiplier);
        xrayObject.endColor.a = (unsigned char)((float)xrayObject.endColor.a * xrayAlphaMultiplier);

        ApplyDebugRenderMode(renderer, xrayObject);
        DrawWorldObject(renderer, font, camera, xrayObject);
    }

    // Second pass: draw all world objects normally.
    for (DebugObject const& obj : s_debugObjects)
    {
        if (obj.type == DebugObjectType::SCREEN_TEXT || obj.type == DebugObjectType::MESSAGE)
        {
            continue;
        }

        ApplyDebugRenderMode(renderer, obj);
        DrawWorldObject(renderer, font, camera, obj);
    }

    ResetRendererStates(renderer);
}

void DebugRenderScreen(const Camera& camera)
{
    if (!s_isVisible)
    {
        return;
    }

    Renderer* renderer = s_debugRenderConfig.m_renderer;
    if (renderer == nullptr)
    {
        return;
    }

    BitmapFont* font = renderer->CreateOrGetBitmapFont(
        Stringf("%s%s", s_debugRenderConfig.m_fontPath.c_str(), s_debugRenderConfig.m_fontName.c_str()).c_str()
    );

    for (DebugObject const& obj : s_debugObjects)
    {
        if (obj.type != DebugObjectType::SCREEN_TEXT)
        {
            continue;
        }

        ApplyDebugRenderMode(renderer, obj);
        DrawScreenObject(renderer, font, camera, obj);
    }

    for (int messageIndex = 0; messageIndex < (int)s_debugMessages.size(); ++messageIndex)
    {
        DebugObject const& obj = s_debugMessages[messageIndex];
        ApplyDebugRenderMode(renderer, obj);
        DrawScreenObject(renderer, font, camera, obj, (int)s_debugMessages.size() - messageIndex - 1);
    }

    ResetRendererStates(renderer);
}

void DebugRenderEndFrame()
{
    float dt = (float)Clock::GetSystemClock().GetDeltaSeconds(); 
    UpdateDebugObjectLifetimes(s_debugObjects, dt);
    UpdateDebugObjectLifetimes(s_debugMessages, dt);
}

bool Command_DebugRenderClear([[maybe_unused]] EventArgs& args)
{
    DebugRenderClear();
    return true;
}

bool Command_DebugRenderToggle([[maybe_unused]] EventArgs& args)
{
    s_isVisible = !s_isVisible;
    return true;
}
