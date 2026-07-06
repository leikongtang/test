"""从 dragon.png 抠出龙身，生成透明背景 dragon_cutout.png"""
from collections import deque
from PIL import Image


def color_dist(a, b):
    return abs(a[0] - b[0]) + abs(a[1] - b[1]) + abs(a[2] - b[2])


def is_dragon_pixel(r, g, b):
    mx, mn = max(r, g, b), min(r, g, b)
    sat = mx - mn
    if g - max(r, b) > 8 and g > 60:
        return True
    if sat > 35 and g >= r - 20:
        return True
    if mx > 180 and sat < 40 and g >= r - 15:
        return True
    return False


def is_background_pixel(r, g, b):
    if is_dragon_pixel(r, g, b):
        return False
    mx, mn = max(r, g, b), min(r, g, b)
    sat = mx - mn
    brightness = (r + g + b) // 3
    if brightness > 165 and sat < 55:
        return True
    if b >= r and b >= g - 10 and brightness > 120 and sat < 70:
        return True
    if brightness > 130 and sat < 30:
        return True
    return False


def extract(path_in, path_out):
    img = Image.open(path_in).convert("RGBA")
    w, h = img.size
    px = img.load()
    labels = [[0] * w for _ in range(h)]
    next_label = 1
    tolerance = 48

    seeds = []
    for x in range(w):
        for y in (0, h - 1):
            seeds.append((x, y))
    for y in range(h):
        for x in (0, w - 1):
            seeds.append((x, y))

    for sx, sy in seeds:
        if labels[sy][sx]:
            continue
        r, g, b, a = px[sx, sy]
        if not is_background_pixel(r, g, b):
            continue
        label = next_label
        next_label += 1
        q = deque([(sx, sy)])
        labels[sy][sx] = label
        while q:
            x, y = q.popleft()
            cr, cg, cb, _ = px[x, y]
            for nx, ny in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
                if nx < 0 or ny < 0 or nx >= w or ny >= h or labels[ny][nx]:
                    continue
                nr, ng, nb, _ = px[nx, ny]
                if is_dragon_pixel(nr, ng, nb):
                    continue
                if color_dist((cr, cg, cb), (nr, ng, nb)) <= tolerance or is_background_pixel(nr, ng, nb):
                    labels[ny][nx] = label
                    q.append((nx, ny))

    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            if is_background_pixel(r, g, b) or labels[y][x]:
                px[x, y] = (r, g, b, 0)
            elif is_dragon_pixel(r, g, b):
                px[x, y] = (r, g, b, 255)
            else:
                alpha = max(40, min(255, 255 - color_dist((r, g, b), (220, 230, 240))))
                px[x, y] = (r, g, b, alpha)

    bbox = img.getbbox()
    if bbox:
        img = img.crop(bbox)
    img.save(path_out)
    print(f"saved {path_out} size={img.size}")


if __name__ == "__main__":
    extract("resources/dragon.png", "resources/dragon_cutout.png")
