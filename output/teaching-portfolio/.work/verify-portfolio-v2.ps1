param([Parameter(Mandatory=$true)][string]$Pptx, [Parameter(Mandatory=$true)][string]$Pdf)
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip=[System.IO.Compression.ZipFile]::OpenRead($Pptx)
$slideStats=@()
$allLinks=@()
$allText=''
$pageTexts=@{}
$pageXml=@{}
$internalTargets=@()
$geometricObjects=@()
try
{
    $slideEntries=@($zip.Entries | Where-Object { $_.FullName -match '^ppt/slides/slide\d+\.xml$' } | Sort-Object { [int]([regex]::Match($_.FullName,'slide(\d+)').Groups[1].Value) })
    if ($slideEntries.Count -ne 10)
    {
        throw "Expected 10 slides; found $($slideEntries.Count)."
    }

    foreach ($entry in $slideEntries)
    {
        $reader=[System.IO.StreamReader]::new($entry.Open())
        try
        {
            [xml]$xml=$reader.ReadToEnd()
        }
        finally
        {
            $reader.Dispose()
        }

        $texts=@($xml.SelectNodes('//*[local-name()="t"]') | ForEach-Object { $_.InnerText })
        $nativeTexts=@($xml.SelectNodes('//*[local-name()="sp"]/*[local-name()="txBody"]')).Count
        $images=@($xml.SelectNodes('//*[local-name()="pic"]')).Count
        $number=[int]([regex]::Match($entry.FullName,'slide(\d+)').Groups[1].Value)
        $pageTexts[$number] = $texts -join ''
        $pageXml[$number] = $xml
        $allText += ($texts -join '') + "`n"
        $slideStats += [PSCustomObject]@{Page=$number; NativeTextObjects=$nativeTexts; Images=$images; Characters=($texts -join '').Length}
        $rels=$zip.GetEntry("ppt/slides/_rels/slide$number.xml.rels")
        $reader=[System.IO.StreamReader]::new($rels.Open())
        try
        {
            [xml]$relsXml=$reader.ReadToEnd()
        }
        finally
        {
            $reader.Dispose()
        }

        foreach ($link in $relsXml.SelectNodes('//*[local-name()="Relationship"]'))
        {
            if ($link.Type -match '/hyperlink$')
            {
                $allLinks += [PSCustomObject]@{Page=$number; Target=$link.Target}
            }

            if ($number -eq 2 -and $link.Type -match '/slide$')
            {
                $internalTargets += [int]([regex]::Match($link.Target,'slide(\d+)\.xml').Groups[1].Value)
            }
        }
    }

    if (!($allLinks | Where-Object { $_.Page -eq 1 -and $_.Target -eq 'https://zhoushangli.github.io/' }))
    {
        throw 'Cover hyperlink missing or incorrect.'
    }

    if ($allText -notmatch '预计 2027 年 5 月毕业')
    {
        throw 'Contact or education wording missing.'
    }

    if ($allText -match 'TODO|TBD|待补|占位文字')
    {
        throw 'Unresolved placeholder found.'
    }

    $notes=@($zip.Entries | Where-Object { $_.FullName -match '^ppt/notesSlides/notesSlide\d+\.xml$' }).Count
    if ($notes -ne 10)
    {
        throw 'Source notes missing.'
    }

    if ($slideStats[0].Images -ne 0 -or $pageTexts[1] -match 'MingEngine|Doomenstein|SpectreX|Hamsterballin|三原色')
    {
        throw 'Cover contains project material.'
    }

    if ($pageTexts[2] -notmatch '作品集大纲' -or $pageTexts[3] -notmatch '个人经历')
    {
        throw 'Outline or experience page is out of order.'
    }

    $expectedProjects=@(
        @{Page=4;Title='MingEngine';Slug='ming-engine';Category='个人作品'},
        @{Page=5;Title='Doomenstein';Slug='doomenstein';Category='个人作品'},
        @{Page=6;Title='The Primary Colors';Slug='the-primary-colors';Category='个人作品'},
        @{Page=7;Title='SpectreX';Slug='spectrex';Category='团队作品'},
        @{Page=8;Title="Hamsterballin'";Slug='hamsterballin';Category='团队作品'}
    )
    foreach ($project in $expectedProjects)
    {
        $visible=$pageTexts[$project.Page]
        $expectedUrl='https://zhoushangli.github.io/projects/'+$project.Slug+'/'
        if (!$visible.Contains($project.Title) -or !$visible.Contains($project.Category) -or !$visible.Contains('我的职责'))
        {
            throw ('Project order/category/role mismatch at page '+$project.Page)
        }

        if (!($allLinks | Where-Object { $_.Page -eq $project.Page -and $_.Target -eq $expectedUrl }))
        {
            throw ('Project link missing at page '+$project.Page)
        }
    }

    foreach ($target in 3..10)
    {
        if ($internalTargets -notcontains $target)
        {
            throw ('Outline internal target missing: '+$target)
        }
    }

    foreach ($teachingLink in @(
        @{Page=9;Url='https://zhoushangli.github.io/teaching/pico8-2026/'},
        @{Page=10;Url='https://zhoushangli.github.io/teaching/pico8-game-jam/'}
    ))
    {
        if (!($allLinks | Where-Object { $_.Page -eq $teachingLink.Page -and $_.Target -eq $teachingLink.Url }))
        {
            throw 'Teaching link missing or on the wrong page.'
        }
    }

    if ($allText -match '拟授|拟定练习|检查方式（拟）')
    {
        throw 'Unrequested proposed teaching content remains.'
    }

    $manifest=Get-Content -LiteralPath 'C:\Work\zhoushangli.github.io\src\data\gamejam.ts' -Raw -Encoding UTF8
    $studentWorks=$manifest.Substring($manifest.IndexOf('['),$manifest.LastIndexOf(']')-$manifest.IndexOf('[')+1) | ConvertFrom-Json
    $pictures=@($pageXml[10].SelectNodes('//*[local-name()="pic"]/*[local-name()="nvPicPr"]/*[local-name()="cNvPr"]'))
    if ($pictures.Count -ne 15 -or $studentWorks.Count -ne 15)
    {
        throw 'Student gallery is not exactly 15 images.'
    }

    $studentTitles=@()
    for ($index=0; $index -lt 15; $index++)
    {
        $expectedName=[System.IO.Path]::GetFileName($studentWorks[$index].cover)
        if ($pictures[$index].descr -ne $expectedName -or !$pageTexts[10].Contains($studentWorks[$index].title))
        {
            throw ('Student gallery mismatch at index '+$index)
        }

        $studentTitles+=$studentWorks[$index].title
    }
}
finally
{
    $zip.Dispose()
}

$pdfRaw=[System.Text.Encoding]::ASCII.GetString([System.IO.File]::ReadAllBytes($Pdf))
$pdfLinks=@([regex]::Matches($pdfRaw,'/URI\s*\(([^)]*)\)') | ForEach-Object { $_.Groups[1].Value } | Sort-Object -Unique)
if ($pdfLinks -notcontains 'https://zhoushangli.github.io/')
{
    throw 'PDF homepage URI missing.'
}

foreach ($requiredLink in $allLinks)
{
    if ($pdfLinks -notcontains $requiredLink.Target)
    {
        throw ('PDF URI not preserved: '+$requiredLink.Target)
    }
}

$pdfInternalLinks=[regex]::Matches($pdfRaw,'/S\s*/GoTo\b|/Dest\s*\[').Count
if ($pdfInternalLinks -lt 8)
{
    throw 'PDF outline navigation annotations missing.'
}

$pdfPageCount=[regex]::Matches($pdfRaw,'/Type\s*/Page\b').Count
if ($pdfPageCount -ne 10)
{
    throw "Expected 10 PDF pages; found $pdfPageCount."
}

$native=New-Object -ComObject PowerPoint.Application
$document=$native.Presentations.Open($Pptx,-1,0,0)
try
{
    $nativeSlideCount=$document.Slides.Count
    $width=$document.PageSetup.SlideWidth
    $height=$document.PageSetup.SlideHeight
    if ([Math]::Abs($width-841.8898) -gt 0.2 -or [Math]::Abs($height-595.2756) -gt 0.2)
    {
        throw 'Slide dimensions are not A4 landscape.'
    }

    $nativeCoverLink=$document.Slides.Item(1).Hyperlinks | ForEach-Object { $_.Address }
    $geometry=@()
    foreach ($slide in $document.Slides)
    {
        foreach ($shape in $slide.Shapes)
        {
            if ($shape.Name -like 'Geometry-*')
            {
                $geometricObjects += [PSCustomObject]@{Page=$slide.SlideIndex;Name=$shape.Name;Type=$shape.Type;FillVisible=$shape.Fill.Visible;LineWidth=$shape.Line.Weight;Transparency=$shape.Line.Transparency}
            }

            if ($shape.HasTextFrame -eq -1 -and $shape.TextFrame.HasText -eq -1)
            {
                if ($shape.TextFrame2.TextRange.BoundHeight -gt $shape.Height+1)
                {
                    $geometry += [PSCustomObject]@{Page=$slide.SlideIndex;Shape=$shape.Name;Problem='Text overflow'}
                }

                if ($shape.Left -lt -1 -or $shape.Top -lt -1 -or $shape.Left+$shape.Width -gt $width+1 -or $shape.Top+$shape.Height -gt $height+1)
                {
                    $geometry += [PSCustomObject]@{Page=$slide.SlideIndex;Shape=$shape.Name;Problem='Outside slide bounds'}
                }
            }
        }
    }
}
finally
{
    $document.Close()
}

if ($geometry.Count -gt 0)
{
    $geometry | ConvertTo-Json -Depth 4
    throw 'Native geometry verification failed.'
}

[PSCustomObject]@{
    Pptx=$Pptx; Pdf=$Pdf; NativePowerPointVersion=$native.Version;
    PowerPointPages=$nativeSlideCount; PdfPages=$pdfPageCount;
    WidthPt=$width; HeightPt=$height; SourcesInNotes=$notes;
    NativeTextObjects=($slideStats | Measure-Object NativeTextObjects -Sum).Sum;
    SlideStats=$slideStats; Hyperlinks=$allLinks; PdfUriTargets=$pdfLinks;
    CoverNativeLinks=@($nativeCoverLink); GeometryFailures=$geometry.Count;
    InternalOutlineTargets=@($internalTargets | Sort-Object -Unique); PdfInternalLinkAnnotations=$pdfInternalLinks;
    StudentWorksInOrder=$studentTitles; EditableGeometry=$geometricObjects;
    PptxBytes=(Get-Item -LiteralPath $Pptx).Length; PdfBytes=(Get-Item -LiteralPath $Pdf).Length;
    DesktopScreenshotAndClickTest='Unavailable: Computer Use native pipe not connected';
    Validation='PASS: 10 pages, native reopen, editable objects, cover, outline, ordered projects, 15 student works, geometry and PDF/PPTX links'
} | ConvertTo-Json -Depth 6
