"""Export real renderer captures; the optional video presents automated checkpoints."""

from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


def main():
    captures = Path(__file__).resolve().parents[1] / "Temporary/NativeFieldsVerification/Captures"
    for source in captures.glob("*.ppm"):
        with Image.open(source) as image:
            image.save(source.with_suffix(".png"))

    try:
        import cv2
        import numpy as np
    except ImportError:
        print(f"PNGs exported to {captures}; install OpenCV to export the checkpoint video.")
        return

    stages = [
        ("startup-manager", "Native project manager"),
        ("01-editor", "Real scene selection and native Inspector"),
        ("02-property-edit", "Edit Position X through native input"),
        ("03-reopened", "Save and reopen the scene"),
        ("04-resized", "Resize layout and scene viewport"),
        ("05-filesystem", "Browse actual project resources"),
        ("06-confirmation", "Native unsaved-scene confirmation"),
    ]
    output = captures / "rendered-checkpoints.mp4"
    video = cv2.VideoWriter(str(output), cv2.VideoWriter_fourcc(*"mp4v"), 12, (1280, 840))
    if not video.isOpened():
        raise RuntimeError("Cannot create the checkpoint video.")

    font = ImageFont.truetype("C:/Windows/Fonts/consola.ttf", 20)
    for name, caption in stages:
        source = captures / f"{name}.png"
        if not source.exists():
            continue

        frame = Image.new("RGB", (1280, 840), (25, 29, 37))
        draw = ImageDraw.Draw(frame)
        draw.text((20, 12), caption, font=font, fill=(225, 230, 240))
        draw.text((20, 806), "Automated renderer checkpoints - not a desktop recording", font=font, fill=(155, 170, 190))
        with Image.open(source) as image:
            image.thumbnail((1240, 740))
            frame.paste(image, ((1280 - image.width) // 2, 54 + (740 - image.height) // 2))

        pixels = cv2.cvtColor(np.asarray(frame), cv2.COLOR_RGB2BGR)
        for _ in range(36):
            video.write(pixels)

    video.release()
    print(f"PNGs and automated checkpoint video exported to {captures}")


if __name__ == "__main__":
    main()
