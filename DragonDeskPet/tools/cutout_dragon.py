"""去除灰白棋盘格背景，输出透明 dragon_cutout.png"""
from PIL import Image


def is_bg(r, g, b):
    mx = max(r, g, b)
    mn = min(r, g, b)
    if mx > 230 and mx - mn < 20:
        return True
    if abs(r - g) < 8 and abs(g - b) < 8 and r > 200:
        return True
    return False


def is_dragon(r, g, b):
    if is_bg(r, g, b):
        return False
    green_dom = g - max(r, b)
    if green_dom > 12 and g > 50:
        return True
    mx, mn = max(r, g, b), min(r, g, b)
    if mx - mn > 30:
        return True
    if mx > 160 and mx - mn < 50:
        return True
    return False


def main():
    src = Image.open("resources/dragon_2d.png").convert("RGBA")
    w, h = src.size
    px = src.load()
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            if is_bg(r, g, b) and not is_dragon(r, g, b):
                px[x, y] = (r, g, b, 0)
            else:
                px[x, y] = (r, g, b, 255)

    bbox = src.getbbox()
    if bbox:
        src = src.crop(bbox)
    src.save("resources/dragon_cutout.png")
    print("saved dragon_cutout.png", src.size)


if __name__ == "__main__":
    main()
