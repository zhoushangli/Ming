#include "Engine/Renderer/DebugRenderer.hpp"

#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/Engine.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
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
        WORLD_BASIS,
        WORLD_TEXT,
        WORLD_BILLBOARD_TEXT,
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

        // basis
        float length = 1.f;
        float colorScale = 1.f;
        float alphaScale = 1.f;
    };

    static DebugRenderConfig s_debugRenderConfig;
    static std::vector<DebugObject> s_debugObjects;
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

    Rgba8 InterpolateColor(DebugObject const& obj)
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
                renderer->SetDepthMode(DepthMode::READ_ONLY_ALWAYS);
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
        (void)camera;

        std::vector<Vertex> verts;
        verts.reserve(2048);

        Rgba8 color = InterpolateColor(obj);

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

        case DebugObjectType::WORLD_BASIS:
        {
            // draw 3 arrows (X=red, Y=green, Z=blue) using transform's translation as origin
            Vec3 origin = obj.transform.GetTranslation3D();

            // Build basis directions from transform by transforming unit axes; assumes transform is orthonormal-ish.
            Vec3 xEnd = obj.transform.TransformPosition3D(Vec3(obj.length, 0.f, 0.f));
            Vec3 yEnd = obj.transform.TransformPosition3D(Vec3(0.f, obj.length, 0.f));
            Vec3 zEnd = obj.transform.TransformPosition3D(Vec3(0.f, 0.f, obj.length));

            Rgba8 cx = Rgba8::RED;   cx.a = (unsigned char)((float)cx.a * obj.alphaScale); cx = cx * obj.colorScale;
            Rgba8 cy = Rgba8::GREEN; cy.a = (unsigned char)((float)cy.a * obj.alphaScale); cy = cy * obj.colorScale;
            Rgba8 cz = Rgba8::BLUE;  cz.a = (unsigned char)((float)cz.a * obj.alphaScale); cz = cz * obj.colorScale;

            // helper multiply operator may not exist; keep safe: scale rgb manually
            auto scaleRgb = [](Rgba8& c, float s)
            {
                c.r = (unsigned char)GetClamped((int)((float)c.r * s), 0, 255);
                c.g = (unsigned char)GetClamped((int)((float)c.g * s), 0, 255);
                c.b = (unsigned char)GetClamped((int)((float)c.b * s), 0, 255);
            };
            scaleRgb(cx, obj.colorScale);
            scaleRgb(cy, obj.colorScale);
            scaleRgb(cz, obj.colorScale);

            AddVertsForArrow3D(verts, origin, xEnd, obj.radius, cx);
            AddVertsForArrow3D(verts, origin, yEnd, obj.radius, cy);
            AddVertsForArrow3D(verts, origin, zEnd, obj.radius, cz);

            renderer->BindTexture(nullptr);
            renderer->SetModelConstants(Matrix4x4::IDENTITY, Rgba8::WHITE);
            renderer->DrawVertexArray(verts);
        }
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

//             font->AddVertsForText3DAtOriginXForward(verts, obj.textHeight, obj.text, color, 1.0f, obj.alignment);
// 
//             Matrix4x4 billboard = GetBillboardTransform(BillboardType::FULL_OPPOSING,
//                 camera.GetCameraToWorldTransform(), obj.center);
//             TransformVertexArray3D(verts, billboard);
// 
//             renderer->BindTexture(&font->GetTexture());
//             renderer->SetModelConstants(Matrix4x4::IDENTITY, Rgba8::WHITE);
//             renderer->DrawVertexArray(verts);
        }
        break;

        default:
            break;
        }
    }

    void DrawScreenObject(Renderer* renderer, BitmapFont* font, DebugObject const& obj)
    {
        if (renderer == nullptr || font == nullptr)
        {
            return;
        }

        if (obj.type != DebugObjectType::SCREEN_TEXT && obj.type != DebugObjectType::MESSAGE)
        {
            return;
        }

        Rgba8 color = InterpolateColor(obj);

        AABB2 box = obj.screenBox;
        if (obj.type == DebugObjectType::MESSAGE)
        {
            // Simple default: message box near top-left
            Vec2 windowDimensions = (Vec2)g_engine->m_window->GetClientDimensions();
            float h = 20.f;
            box = AABB2(Vec2(10.f, windowDimensions.y - 10.f - h), Vec2(windowDimensions.x - 10.f, windowDimensions.y - 10.f));
            // alignment top-left-ish
        }

        std::vector<Vertex> verts;
        verts.reserve(1024);

        font->AddVertsForTextInBox2D(verts, obj.text, box, obj.textHeight, color, 1.f, obj.alignment, TextBoxMode::SHRINK_TO_FIT);

        renderer->BindTexture(&font->GetTexture());
        renderer->SetModelConstants(Matrix4x4::IDENTITY, Rgba8::WHITE);
        renderer->DrawVertexArray(verts);
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
    s_isVisible = true;
}

void DebugRenderSystemShutdown()
{
    s_debugRenderConfig.m_renderer = nullptr;
    s_debugObjects.clear();
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
    DebugObject object = MakeDebugObject(DebugObjectType::WORLD_BASIS, duration, Rgba8::WHITE, Rgba8::WHITE, mode);
    object.transform = transform;
    object.length = length;
    object.radius = radius;
    object.colorScale = colorScale;
    object.alphaScale = alphaScale;
    s_debugObjects.push_back(object);
}

void DebugAddWorldBasis(const Matrix4x4& transform, float duration, DebugRenderMode mode)
{
    DebugAddBasis(transform, duration, 1.0f, 0.025f, 1.0f, 1.0f, mode);
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

}

void DebugAddMessage(const std::string& text, float cellHeight, const Vec2& alignment, float duration, const Rgba8& startColor /*= Rgba8::WHITE*/, const Rgba8& endColor /*= Rgba8::WHITE*/)
{
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
        xrayObject.startColor.a = (unsigned char)((float)xrayObject.startColor.a * 0.35f);
        xrayObject.endColor.a = (unsigned char)((float)xrayObject.endColor.a * 0.35f);

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
    (void)camera;

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
        if (obj.type != DebugObjectType::SCREEN_TEXT && obj.type != DebugObjectType::MESSAGE)
        {
            continue;
        }

        ApplyDebugRenderMode(renderer, obj);
        DrawScreenObject(renderer, font, obj);
    }

    ResetRendererStates(renderer);
}

void DebugRenderEndFrame()
{
    float dt = (float)Clock::GetSystemClock().GetDeltaSeconds(); 

    for (int i = (int)s_debugObjects.size() - 1; i >= 0; --i)
    {
        DebugObject& obj = s_debugObjects[i];

        if (obj.totalDuration < 0.f)
        {
            continue;
        }

        obj.remainingDuration -= dt;

        if (obj.remainingDuration <= 0.f)
        {
            s_debugObjects.erase(s_debugObjects.begin() + i);
        }
    }
}

// bool Command_DebugRenderClear([[maybe_unused]] EventArgs& args)
// {
//     return false;
// }
// 
// bool Command_DebugRenderToggle([[maybe_unused]] EventArgs& args)
// {
//     return false;
// }
