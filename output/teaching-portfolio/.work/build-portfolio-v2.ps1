param(
    [switch]$Samples,
    [string]$Revision = 'v2-r1'
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$script:Root = 'C:\Work\Ming\Ming\output\teaching-portfolio'
$script:Site = 'C:\Work\zhoushangli.github.io'
$script:Work = Join-Path $script:Root '.work\v2'
$script:Render = Join-Path $script:Work ($Revision + $(if ($Samples) { '-samples' } else { '-full' }))
New-Item -ItemType Directory -Force -Path $script:Root, $script:Render | Out-Null
$script:TextColor = 'F0E9DC'
$script:Muted = 'BBB9B1'
$script:Copper = 'D2B387'
$script:Bg = '101722'
$script:Issues = [System.Collections.Generic.List[object]]::new()
$script:TextCount = 0
$script:ImageCount = 0

function Color([string]$Hex)
{
    return [Convert]::ToInt32($Hex.Substring(0,2),16) + 256 * [Convert]::ToInt32($Hex.Substring(2,2),16) + 65536 * [Convert]::ToInt32($Hex.Substring(4,2),16)
}

function Text([string]$Value, [double]$X, [double]$Y, [double]$W, [double]$H, [double]$Size = 16, [string]$Ink = '', [string]$Face = '微软雅黑', [string]$Url = '', [switch]$Bold)
{
    if (!$Ink)
    {
        $Ink = $script:TextColor
    }

    $shape = $script:Slide.Shapes.AddTextbox(1, $X, $Y, $W, $H)
    $shape.Name = 'Text-' + (++$script:TextCount)
    $shape.TextFrame.MarginLeft = 0
    $shape.TextFrame.MarginRight = 0
    $shape.TextFrame.MarginTop = 0
    $shape.TextFrame.MarginBottom = 0
    $shape.TextFrame.WordWrap = -1
    $shape.TextFrame.AutoSize = 0
    $range = $shape.TextFrame.TextRange
    $range.Text = $Value
    $range.Font.Name = $Face
    $range.Font.NameFarEast = $Face
    $range.Font.Size = $Size
    $range.Font.Color.RGB = Color $Ink
    $range.Font.Bold = $(if ($Bold) { -1 } else { 0 })
    $range.ParagraphFormat.SpaceAfter = 0
    $range.ParagraphFormat.SpaceBefore = 0
    $range.ParagraphFormat.SpaceWithin = 1.18
    if ($Value.StartsWith('我的职责：'))
    {
        $range.Characters(1,5).Font.Color.RGB = Color $script:Copper
    }

    if ($Url)
    {
        $shape.ActionSettings.Item(1).Action = 7
        $shape.ActionSettings.Item(1).Hyperlink.Address = $Url
        $range.ActionSettings.Item(1).Action = 7
        $range.ActionSettings.Item(1).Hyperlink.Address = $Url
        $range.Font.Underline = -1
    }

    $shape.Height = $H
    $script:LatestText = $shape
}

function Rule([double]$X, [double]$Y, [double]$W, [string]$Ink = '394451', [double]$Weight = 0.65)
{
    $line = $script:Slide.Shapes.AddLine($X,$Y,($X+$W),$Y)
    $line.Line.ForeColor.RGB = Color $Ink
    $line.Line.Weight = $Weight
}

function Picture([string]$Relative, [double]$X, [double]$Y, [double]$W, [double]$H, [switch]$TrimWindow)
{
    $file = Join-Path $script:Site $Relative
    if ($TrimWindow)
    {
        $original = [System.Drawing.Image]::FromFile($file)
        $topCrop = [int][Math]::Round($original.Height*0.0375)
        $cropped = [System.Drawing.Bitmap]::new($original.Width-4,$original.Height-$topCrop-2)
        $graphics = [System.Drawing.Graphics]::FromImage($cropped)
        $graphics.DrawImage($original,[System.Drawing.Rectangle]::new(0,0,$cropped.Width,$cropped.Height),[System.Drawing.Rectangle]::new(2,$topCrop,$cropped.Width,$cropped.Height),[System.Drawing.GraphicsUnit]::Pixel)
        $file = Join-Path $script:Work ([System.IO.Path]::GetFileNameWithoutExtension($file) + '-cropped.png')
        $cropped.Save($file,[System.Drawing.Imaging.ImageFormat]::Png)
        $graphics.Dispose()
        $cropped.Dispose()
        $original.Dispose()
    }

    $im = [System.Drawing.Image]::FromFile($file)
    try
    {
        $scale = [Math]::Min($W/$im.Width,$H/$im.Height)
        $iw = $im.Width*$scale
        $ih = $im.Height*$scale
    }
    finally
    {
        $im.Dispose()
    }

    $pic = $script:Slide.Shapes.AddPicture($file,0,-1,($X+($W-$iw)/2),($Y+($H-$ih)/2),$iw,$ih)
    $pic.Name = 'Image-' + (++$script:ImageCount)
    $pic.AlternativeText = [System.IO.Path]::GetFileName($file)
}

function Heading([string]$Section, [string]$Title, [int]$Page)
{
    Text $Section 46 29 650 24 12 $script:Copper
    Text $Title 46 62 750 55 32 $script:TextColor '宋体'
    Rule 46 126 750 $script:Copper 0.9
    Footer $Page
}

function Footer([int]$Page)
{
    Rule 46 550 750
    Text '周尚礼  /  游戏开发与教学实践' 46 562 640 18 10.5 $script:Muted
    Text ('{0:00} / 10' -f $Page) 752 560 60 22 11 $script:Copper
}

function Notes([string]$Source)
{
    $script:Slide.NotesPage.Shapes.Placeholders.Item(2).TextFrame.TextRange.Text = $Source
}

function GeometryShape([int]$Kind, [double]$X, [double]$Y, [double]$W, [double]$H)
{
    $shape = $script:Slide.Shapes.AddShape($Kind,$X,$Y,$W,$H)
    $shape.Name = 'Geometry-' + $script:Slide.Shapes.Count
    $shape.Fill.Visible = 0
    $shape.Line.ForeColor.RGB = Color $script:Copper
    $shape.Line.Weight = 0.65
    $shape.Line.Transparency = 0.75
}

function GeometryLine([double]$X1, [double]$Y1, [double]$X2, [double]$Y2)
{
    $shape = $script:Slide.Shapes.AddLine($X1,$Y1,$X2,$Y2)
    $shape.Name = 'Geometry-' + $script:Slide.Shapes.Count
    $shape.Line.ForeColor.RGB = Color $script:Copper
    $shape.Line.Weight = 0.65
    $shape.Line.Transparency = 0.75
}

function ChapterGeometry([int]$Chapter)
{
    switch ($Chapter)
    {
        1
        {
            GeometryShape 9 690 -65 190 190
            GeometryShape 9 714 -41 142 142
        }
        2
        {
            GeometryShape 1 715 -28 145 110
            GeometryShape 1 740 -8 145 110
        }
        3
        {
            GeometryShape 9 700 -34 130 130
            GeometryShape 9 759 -34 130 130
        }
        default
        {
            GeometryLine 46 46 104 46
            GeometryLine 46 46 46 80
            GeometryLine 738 548 796 548
            GeometryLine 796 514 796 548
        }
    }
}

function ToSlide([string]$Value, [double]$X, [double]$Y, [double]$W, [double]$H, [int]$Target, [double]$Size=16, [string]$Ink='', [string]$Face='微软雅黑')
{
    Text $Value $X $Y $W $H $Size $Ink $Face
    $script:PendingLinks.Add([PSCustomObject]@{Shape=$script:LatestText;Target=$Target})
}

function OutlineEntry([string]$Title, [int]$Target, [double]$Y)
{
    ToSlide $Title 330 $Y 400 27 $Target 16 $script:TextColor
    ToSlide ('{0:00}' -f $Target) 754 $Y 42 27 $Target 15 $script:Copper
}

function ProjectLink([string]$Slug)
{
    $url = 'https://zhoushangli.github.io/projects/' + $Slug + '/'
    Text ('博客项目页  /  zhoushangli.github.io/projects/' + $Slug + '/') 46 525 750 22 11.5 $script:Copper '微软雅黑' $url
}

function KeyPoint([int]$Index, [string]$Title, [string]$Body)
{
    $top = 214 + ($Index-1)*72
    Text ('0' + $Index) 480 ($top+2) 25 24 11 $script:Copper
    Text $Title 509 $top 287 29 18 $script:TextColor '宋体'
    Text $Body 509 ($top+31) 287 42 15 $script:Muted
}

function CourseRow([string]$Number, [string]$Title, [string]$Body, [double]$Y)
{
    Text $Number 330 $Y 58 27 18 $script:Copper '宋体'
    Text $Title 398 $Y 398 28 19 $script:TextColor '宋体'
    Text $Body 398 ($Y+32) 398 38 15 $script:Muted
    Rule 330 ($Y+74) 466
}

$script:PendingLinks = [System.Collections.Generic.List[object]]::new()
$script:SlideMap = @{}
$script:Ppt = New-Object -ComObject PowerPoint.Application
$script:Deck = $script:Ppt.Presentations.Add(0)
$script:Deck.PageSetup.SlideWidth = 841.8898
$script:Deck.PageSetup.SlideHeight = 595.2756
$script:Deck.SlideMaster.Theme.ThemeColorScheme.Colors(11).RGB = Color $script:Copper
$script:Deck.SlideMaster.Theme.ThemeColorScheme.Colors(12).RGB = Color $script:Copper
$pages = @(1..10)
if ($Samples)
{
    $pages = @(1,2,4,9)
}

foreach ($page in $pages)
{
    $script:Slide = $script:Deck.Slides.Add($script:Deck.Slides.Count+1,12)
    $script:SlideMap[$page] = $script:Slide
    $script:Slide.FollowMasterBackground = 0
    $script:Slide.Background.Fill.Solid()
    $script:Slide.Background.Fill.ForeColor.RGB = Color $script:Bg
    $chapter = 0
    if ($page -eq 3)
    {
        $chapter = 1
    }
    elseif ($page -ge 4 -and $page -le 8)
    {
        $chapter = 2
    }
    elseif ($page -ge 9)
    {
        $chapter = 3
    }

    ChapterGeometry $chapter
    switch ($page)
    {
        1
        {
            Text '周尚礼' 92 153 650 80 46 $script:TextColor '宋体'
            Text '个人作品集' 94 247 650 64 36 $script:TextColor '宋体'
            Rule 94 343 100 $script:Copper 0.8
            Text '我的个人网站收录了作品的完整介绍、演示与相关记录：' 94 382 650 53 16 $script:Muted
            Text 'https://zhoushangli.github.io/' 94 454 650 34 18 $script:Copper '微软雅黑' 'https://zhoushangli.github.io/'
            Text '01 / 10' 752 560 60 22 11 $script:Copper
            Notes '封面按用户指定，只展示姓名、作品集身份、网站说明和真实超链接，不放具体作品图片。个人主页：https://zhoushangli.github.io/'
        }
        2
        {
            Heading '阅读导航' '作品集大纲' 2
            ToSlide '01' 46 157 45 32 3 22 $script:Copper '宋体'
            ToSlide '个人经历' 103 156 197 35 3 24 $script:TextColor '宋体'
            OutlineEntry '教育、实习与个人总览' 3 160
            Rule 46 214 750
            ToSlide '02' 46 238 45 32 4 22 $script:Copper '宋体'
            ToSlide '具体作品' 103 237 197 35 4 24 $script:TextColor '宋体'
            ToSlide '个人作品' 330 235 400 26 4 14 $script:Copper
            OutlineEntry 'MingEngine' 4 268
            OutlineEntry 'Doomenstein' 5 300
            OutlineEntry 'The Primary Colors / 三原色' 6 332
            ToSlide '团队作品' 330 374 400 26 7 14 $script:Copper
            OutlineEntry 'SpectreX' 7 406
            OutlineEntry "Hamsterballin'" 8 438
            Rule 46 478 750
            ToSlide '03' 46 494 45 32 9 22 $script:Copper '宋体'
            ToSlide '教学实践' 103 493 197 35 9 24 $script:TextColor '宋体'
            OutlineEntry 'PICO-8 培训总览' 9 492
            OutlineEntry '培训后的学生作品' 10 521
            Notes "章节与个人/团队顺序依据用户指定。项目顺序依据 $script:Site\src\data\projects.ts 及src\pages\projects\index.astro按personal、team分类后的原始顺序。各条目和页码设置内部跳转，完整版本目标为第3至10页。"
        }
        3
        {
            Heading '第一章  /  个人经历' '个人经历' 3
            Picture 'src\content\images\hero_image.jpg' 46 153 171 255
            Text '游戏程序、玩法与关卡设计' 251 153 545 32 22 $script:TextColor '宋体'
            Text '我本科就读于上海科技大学计算机科学专业，目前在 SMU Guildhall 学习游戏开发。我使用 C++ 开发 MingEngine，也在 Godot、Unity 和 Unreal Engine 中参与个人与团队制作，并为游戏社新成员主讲入门培训。' 251 201 545 94 15.5 $script:Muted
            Rule 251 308 545
            Text '教育经历' 251 326 240 28 18 $script:Copper '宋体'
            Text "2025–2027  SMU Guildhall`n游戏开发研究生课程，程序方向`n预计 2027 年 5 月毕业" 251 365 270 81 14 $script:TextColor
            Text "2021–2025  上海科技大学`n计算机科学本科" 552 365 244 81 14 $script:TextColor
            Rule 46 468 750
            Text '实习经历' 46 487 170 28 18 $script:Copper '宋体'
            Text "腾讯魔方工作室群 / 战斗策划实习`n2024.01–02  音效与敌人战斗动画配置" 236 486 295 58 13 $script:Muted
            Text "朝夕光年 / 软件开发实习`n2023.08–12  PyQt 引擎工具链界面" 552 486 244 58 13 $script:Muted
            Notes "来源：$script:Site\src\pages\index.astro、src\components\ExperienceTimeline.astro。保留预计2027年5月毕业表述，不推定已获得学位；实习职责按已发布内容整理。头像为网站原图。"
        }
        4
        {
            Heading '第二章  /  具体作品  /  个人作品' 'MingEngine' 4
            Text '我使用 C++ 开发的 3D 引擎与编辑器，采用 DirectX 11 渲染，支持 C#/.NET 脚本。' 46 148 750 48 15.5 $script:Muted
            Picture 'src\content\images\ming_engine.png' 46 216 407 232
            Text "较早开发阶段的编辑器截图。`n原生 UI 与导出流程仍在完善。" 46 460 407 37 11 $script:Muted
            KeyPoint 1 '节点与资源' '将场景对象、序列化数据和渲染提交分开。'
            KeyPoint 2 'DirectX 11 渲染' '材质纹理、方向光与点光源、粒子及后处理。'
            KeyPoint 3 '编辑器与资源导入' '层级、属性检查与 Gizmo，支持资源导入。'
            KeyPoint 4 'C#/.NET 集成' '通过 hostfxr 与生成绑定接入 C# 脚本。'
            Text '我的职责：独立完成架构、渲染、资源管线、编辑器与脚本集成。' 46 501 750 22 12 $script:TextColor
            ProjectLink 'ming-engine'
            Notes "事实与职责来源：$script:Site\src\data\projects.ts，ming-engine条目。图片：src\content\images\ming_engine.png，较早开发阶段。https://zhoushangli.github.io/projects/ming-engine/"
        }
        5
        {
            Heading '第二章  /  具体作品  /  个人作品' 'Doomenstein' 5
            Text '基于自研 C++ 引擎制作的复古 3D 第一人称射击原型，支持本地双人游玩。' 46 148 750 48 15.5 $script:Muted
            Picture 'src\content\images\doomenstein_game.png' 46 216 407 232 -TrimWindow
            Text '自研引擎中的游戏画面。' 46 460 407 37 11 $script:Muted
            KeyPoint 1 '八方向精灵' '用不同朝向的精灵表现复古角色与敌人。'
            KeyPoint 2 '本地双人输入' '组织玩家实例，支持键鼠与 Xbox 手柄。'
            KeyPoint 3 '模型与简单 IK' '加载 OBJ 模型，为蜘蛛敌人实现简单 IK。'
            KeyPoint 4 '粒子与后处理' '结合 CPU 粒子、体积云和自发光塑造场景。'
            Text '我的职责：独立完成玩法框架、渲染、输入、敌人行为与场景内容。' 46 501 750 22 12 $script:TextColor
            ProjectLink 'doomenstein'
            Notes "来源：$script:Site\src\data\projects.ts，doomenstein条目。原图src\content\images\doomenstein_game.png，仅去除系统窗口边框，不修改游戏内容。不将该项目的自研引擎未经证实地等同于MingEngine。https://zhoushangli.github.io/projects/doomenstein/"
        }
        6
        {
            Heading '第二章  /  具体作品  /  个人作品' 'The Primary Colors / 三原色' 6
            Text '独立开发的平台解谜游戏：通过接触不同颜色改变能力，再运用能力解决关卡中的挑战。' 46 148 750 48 15.5 $script:Muted
            Picture 'src\content\images\the_primary_colors.png' 46 216 407 232
            Text '游戏中的关卡选择画面。' 46 460 407 37 11 $script:Muted
            KeyPoint 1 '颜色能力机制' '将颜色切换与移动、场景交互结合。'
            KeyPoint 2 '30 个手工关卡' '逐步引入机制，再组织不同规则的组合。'
            KeyPoint 3 'MVC 与 QFramework' '分开组织数据、表现与游戏控制逻辑。'
            KeyPoint 4 '事件与命令' '通过事件解耦系统，以命令封装游戏行为。'
            Text '我的职责：独立完成机制、程序架构、表现层和全部 30 个关卡。' 46 501 750 22 12 $script:TextColor
            ProjectLink 'the-primary-colors'
            Notes "来源：$script:Site\src\data\projects.ts，the-primary-colors条目。原图src\content\images\the_primary_colors.png。30个关卡和独立制作范围按网站已发布内容保留。https://zhoushangli.github.io/projects/the-primary-colors/"
        }
        7
        {
            Heading '第二章  /  具体作品  /  团队作品' 'SpectreX' 7
            Text '团队使用 Godot 与 C# 制作的光学解谜游戏，通过反射、分光与合光组织机关和探索。' 46 148 750 48 15.5 $script:Muted
            Picture 'public\media\projects\spectrex-cover.jpg' 46 216 407 232
            Text '团队项目宣传封面。' 46 460 407 37 11 $script:Muted
            KeyPoint 1 '光学解谜规则' '参与玩法简化，将 16 个光频率减少到 3 个。'
            KeyPoint 2 '三章关卡设计' '组织章节行进方式、房间教学与挑战变化。'
            KeyPoint 3 '试玩与迭代' '观察约 10 小时深度试玩，调整难度与引导。'
            KeyPoint 4 '多阶段 2D 渲染' '使用 SubViewport 与 Shader 分层处理及合成。'
            Text '我的职责：全部三章的关卡设计，参与玩法简化、程序与技术美术。' 46 501 750 22 12 $script:TextColor
            ProjectLink 'spectrex'
            Notes "来源：$script:Site\src\data\projects.ts，spectrex条目。团队整体成果与个人职责分开。约10小时指本人观察深度试玩的时长，不是测试者数量。分层渲染采用SubViewport与CanvasItem Shader。https://zhoushangli.github.io/projects/spectrex/"
        }
        8
        {
            Heading '第二章  /  具体作品  /  团队作品' "Hamsterballin'" 8
            Text 'SMU Guildhall 团队使用 Unreal Engine 5 制作的竞技街机赛车游戏。' 46 148 750 48 15.5 $script:Muted
            Picture 'src\content\images\hamsterballin.jpg' 81 246 337 158
            Text '团队游戏宣传封面。' 46 460 407 37 11 $script:Muted
            KeyPoint 1 'PCG 道路生成' '开发程序化道路生成流程，支持赛道制作。'
            KeyPoint 2 '生成性能优化' '优化 PCG 流程，使其能够用于内容生产。'
            KeyPoint 3 '材质与视觉效果' '与美术合作搭建材质，并提供技术支持。'
            KeyPoint 4 '部分 UI 实现' '实现并协助完善选定的界面功能。'
            Text '我的职责：游戏程序，主要负责 PCG 道路工具与优化，并参与技术美术和 UI。' 46 501 750 22 12 $script:TextColor
            ProjectLink 'hamsterballin'
            Notes "来源：$script:Site\src\data\projects.ts，hamsterballin条目。图片为团队游戏封面，原始460x215，未用生成图补造实际游戏画面或性能指标。https://zhoushangli.github.io/projects/hamsterballin/"
        }
        9
        {
            Heading '第三章  /  教学实践' 'PICO-8 入门培训' 9
            Text "面向游戏社新成员的`n社团入门培训" 46 154 250 68 23 $script:TextColor '宋体'
            Text '我主讲 Intro 与三次正课，每次约 90 分钟。课堂结合讲解与跟随操作，课后安排答疑与练习。' 46 241 250 95 16 $script:Muted
            Text '配套练习：打砖块' 46 366 250 31 20 $script:Copper '宋体'
            Text '以小球移动、边缘碰撞和声音为练习，逐步完善游戏。' 46 411 250 65 15 $script:Muted
            Text '助教与其他分享者也参与支持。' 46 489 265 20 11 $script:Muted
            CourseRow 'Intro' '认识工具与制作流程' '保存、运行项目，了解课程与 Gamejam 安排。' 154
            CourseRow 'L01' '绘制、输入与角色移动' '连接变量、按钮输入与角色在画面中的移动。' 238
            CourseRow 'L02' '调试、移动函数与动画' '练习移动函数与动画，通过调试定位问题。' 322
            CourseRow 'L03' '对象、地图与关卡' '使用对象与地图组织内容，制作游戏关卡。' 406
            Text '培训与录制  /  zhoushangli.github.io/teaching/pico8-2026/' 46 525 750 22 11.5 $script:Copper '微软雅黑' 'https://zhoushangli.github.io/teaching/pico8-2026/'
            Notes "来源：$script:Site\src\content\published\teaching\pico8-2026.zh.md（draft:false）。仅整理实际社团培训经历，不加入拟授课程或未经证实的学习成效。https://zhoushangli.github.io/teaching/pico8-2026/"
        }
        10
        {
            Heading '第三章  /  教学实践' 'PICO-8 培训后的学生作品' 10
            Text '培训结束后，社团举办了一周 Gamejam。以下作品归各自创作者。' 46 148 750 27 15 $script:Muted
            $manifestPath = Join-Path $script:Site 'src\data\gamejam.ts'
            $manifest = Get-Content -LiteralPath $manifestPath -Raw -Encoding UTF8
            $manifestJson = $manifest.Substring($manifest.IndexOf('['),$manifest.LastIndexOf(']')-$manifest.IndexOf('[')+1)
            $submissions = $manifestJson | ConvertFrom-Json
            if ($submissions.Count -ne 15)
            {
                throw 'Expected 15 student works from the published manifest.'
            }

            for ($index=0; $index -lt $submissions.Count; $index++)
            {
                $row = [Math]::Floor($index/5)
                $column = $index % 5
                $left = 46 + $column*150
                $top = 176 + $row*110
                Picture ('public' + $submissions[$index].cover.Replace('/','\')) ($left+39) $top 72 82
                Text $submissions[$index].title ($left+5) ($top+88) 140 22 11.5 $script:TextColor
                $script:LatestText.TextFrame.TextRange.ParagraphFormat.Alignment = 2
            }

            Text '完整作品展  /  zhoushangli.github.io/teaching/pico8-game-jam/' 46 525 750 22 11.5 $script:Copper '微软雅黑' 'https://zhoushangli.github.io/teaching/pico8-game-jam/'
            Notes "来源：$script:Site\src\content\published\teaching\pico8-game-jam.zh.md与src\data\gamejam.ts。按博客清单原顺序嵌入全部15个封面，图像未修改。作品归各自创作者，不冒充本人独立作品。https://zhoushangli.github.io/teaching/pico8-game-jam/"
        }
    }
}

$resolved = 0
foreach ($pending in $script:PendingLinks)
{
    if ($script:SlideMap.ContainsKey($pending.Target))
    {
        $target = $script:SlideMap[$pending.Target]
        $subaddress = [string]$target.SlideID + ',' + $target.SlideIndex + ',第' + $pending.Target + '页'
        $action = $pending.Shape.ActionSettings.Item(1)
        $action.Action = 7
        $action.Hyperlink.Address = ''
        $action.Hyperlink.SubAddress = $subaddress
        $pending.Shape.TextFrame.TextRange.Font.Underline = 0
        $resolved++
    }
}

$base = 'portfolio-' + $Revision
$pptx = Join-Path $script:Work ($base + '.pptx')
$pdf = Join-Path $script:Work ($base + '.pdf')
$script:Deck.SaveAs($pptx,24)
$script:Deck.SaveAs($pdf,32)
foreach ($slide in $script:Deck.Slides)
{
    $logicalPage = $pages[$slide.SlideIndex-1]
    $slide.Export((Join-Path $script:Render ('page-{0:00}.png' -f $logicalPage)),'PNG',1684,1191)
    foreach ($shape in $slide.Shapes)
    {
        if ($shape.HasTextFrame -eq -1 -and $shape.TextFrame.HasText -eq -1)
        {
            if ($shape.TextFrame2.TextRange.BoundHeight -gt ($shape.Height+1))
            {
                $script:Issues.Add([PSCustomObject]@{Page=$logicalPage;Shape=$shape.Name;Height=$shape.Height;TextHeight=$shape.TextFrame2.TextRange.BoundHeight;Text=$shape.TextFrame.TextRange.Text})
            }
        }
    }
}

$report = [PSCustomObject]@{Pptx=$pptx;Pdf=$pdf;Render=$script:Render;Pages=$pages;TextObjects=$script:TextCount;Images=$script:ImageCount;InternalLinksResolved=$resolved;InternalLinksPending=$script:PendingLinks.Count-$resolved;Overflow=@($script:Issues.ToArray())}
$report | ConvertTo-Json -Depth 5 | Set-Content -Encoding UTF8 (Join-Path $script:Work ($base + '-geometry.json'))
$script:Deck.Close()
$report | ConvertTo-Json -Depth 5

