"""Create the project PNG and multi-resolution Windows ICO from a square source image."""

from __future__ import annotations

import argparse
from collections import deque
from pathlib import Path

from PIL import Image, ImageFilter


def remove_connected_light_background(image: Image.Image) -> Image.Image:
    rgba = image.convert("RGBA")
    rgb = rgba.convert("RGB")
    width, height = rgb.size
    pixels = rgb.load()

    corners = [pixels[0, 0], pixels[width - 1, 0], pixels[0, height - 1], pixels[width - 1, height - 1]]
    key = tuple(sum(color[channel] for color in corners) // len(corners) for channel in range(3))

    def is_background(x: int, y: int) -> bool:
        color = pixels[x, y]
        return max(abs(color[channel] - key[channel]) for channel in range(3)) <= 150

    outside = Image.new("L", (width, height), 0)
    outside_pixels = outside.load()
    queue: deque[tuple[int, int]] = deque()

    for x in range(width):
        queue.append((x, 0))
        queue.append((x, height - 1))
    for y in range(height):
        queue.append((0, y))
        queue.append((width - 1, y))

    while queue:
        x, y = queue.popleft()
        if outside_pixels[x, y] or not is_background(x, y):
            continue
        outside_pixels[x, y] = 255
        if x > 0:
            queue.append((x - 1, y))
        if x + 1 < width:
            queue.append((x + 1, y))
        if y > 0:
            queue.append((x, y - 1))
        if y + 1 < height:
            queue.append((x, y + 1))

    outside = outside.filter(ImageFilter.GaussianBlur(radius=0.8))
    alpha = outside.point(lambda value: 255 - value)
    rgba.putalpha(alpha)
    return rgba


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--png", type=Path, required=True)
    parser.add_argument("--ico", type=Path, required=True)
    args = parser.parse_args()

    with Image.open(args.source) as source:
        icon = remove_connected_light_background(source)

    args.png.parent.mkdir(parents=True, exist_ok=True)
    png = icon.resize((256, 256), Image.Resampling.LANCZOS)
    png.save(args.png, format="PNG", optimize=True)
    icon.save(
        args.ico,
        format="ICO",
        sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)],
    )

    print(f"PNG: {args.png} ({png.size[0]}x{png.size[1]})")
    print(f"ICO: {args.ico}")


if __name__ == "__main__":
    main()

