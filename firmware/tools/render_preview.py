"""Render an approximate v0.7 design preview of the 320x240 TFT UI.

This preview approximates GFX font metrics; the actual device is authoritative.
The output is a QA mock, not a substitute for running on the ESP32-S3.
"""

from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
import math

ROOT = Path(__file__).resolve().parent.parent
S = 3
W, H = 320, 240
BG = (255, 248, 241)
PAPER = (255, 255, 255)
INK = (43, 49, 69)
MUTED = (126, 130, 148)
CORAL = (236, 119, 112)
MINT = (189, 238, 220)
TEAL = (53, 142, 123)
PEACH = (255, 220, 206)
SHADOW = (238, 226, 221)
TRACK = (238, 237, 239)

FONTPATH = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
FONTBOLD = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"


def f(size, bold=False):
    return ImageFont.truetype(FONTBOLD if bold else FONTPATH, size * S)


def text(draw, x, y, value, size=9, color=INK, bold=False):
    draw.text((x * S, y * S), value, font=f(size, bold), fill=color)


def rr(draw, box, radius, color):
    x0, y0, x1, y1 = box
    draw.rounded_rectangle((x0*S, y0*S, x1*S, y1*S),
                           radius=radius*S, fill=color)


def bar(draw, x, y, w, h, progress, fg=CORAL):
    rr(draw, (x, y, x+w, y+h), h//2, TRACK)
    if progress:
        rr(draw, (x, y, x + int(w*progress), y+h), h//2, fg)


def page(title, sprite, index, mode="DEMO"):
    pic = Image.new("RGBA", (W*S, H*S), BG+(255,))
    d = ImageDraw.Draw(pic)
    rr(d, (9,7,235,37), 15, PAPER)
    d.ellipse((16*S,19*S,22*S,25*S), fill=CORAL)
    text(d, 30, 8, "mimi.", 18, CORAL, True)
    text(d, 100, 16, "DESK OS / 0.7", 8, MUTED)
    rr(d, (244,9,309,34), 12, MINT if mode == "LIVE" else PEACH)
    text(d, 266, 17, mode, 8, TEAL if mode == "LIVE" else CORAL, True)
    text(d, 14, 43, title, 15, INK, True)
    with Image.open(ROOT / "images" / sprite) as source:
        cat = source.convert("RGBA").resize((144*S,144*S),
                                            Image.Resampling.LANCZOS)
    pic.alpha_composite(cat, (176*S,52*S))
    d = ImageDraw.Draw(pic)
    rr(d, (13,80,175,200), 15, SHADOW)
    rr(d, (10,76,172,196), 15, PAPER)
    text(d, 11, 219, "web / n-p / m", 7, MUTED)
    for i in range(12):
        x = 144 + i*11
        if i == index:
            rr(d, (x,219,x+10,225), 3, CORAL)
        else:
            d.ellipse((x*S,220*S,(x+5)*S,225*S), fill=SHADOW)
    text(d, 282, 218, f"{index+1}/12", 8, MUTED)
    return pic, d


def pc_monitor():
    # Every numeric value here is synthetic, visibly marked DEMO.
    pic, d = page("PC MONITOR", "music.png", 0, "DEMO")
    text(d, 19, 84, "MEO DEMO / MOCK", 8, CORAL)
    text(d, 19, 100, "CPU", 9, MUTED)
    text(d, 122, 100, "42%", 9, INK, True)
    bar(d, 19, 114, 135, 6, 0.42, TEAL)
    text(d, 19, 125, "GPU", 9, MUTED)
    text(d, 122, 125, "68%", 9, INK, True)
    bar(d, 19, 140, 135, 6, 0.68, (176, 154, 234))
    text(d, 19, 150, "RAM", 9, MUTED)
    text(d, 122, 150, "53%", 9, INK, True)
    bar(d, 19, 166, 135, 6, 0.53, CORAL)
    text(d, 19, 181, "SIMULATED / NOT REAL", 7, MUTED)
    return pic


def clock():
    pic, d = page("GOOD MORNING", "clock.png", 1)
    text(d, 19, 86, "Thu, 19 Nov", 9, MUTED)
    text(d, 16, 105, "08:45", 34, INK, True)
    rr(d, (18,142,121,164), 11, MINT)
    text(d, 28, 147, "SECONDS 36", 9, TEAL, True)
    text(d, 19, 176, "Time preview", 9, MUTED)
    return pic


def health(too_hot=False):
    cat = "health.png" if too_hot else "healthy.png"
    pic, d = page("PC HEALTH", cat, 10, "DEMO")
    text(d, 19, 84, "HEALTH SENSOR / MOCK", 8, CORAL)
    text(d, 19, 105, "CPU TEMP", 9, MUTED)
    text(d, 120, 105, "88C" if too_hot else "49C", 9, INK, True)
    text(d, 19, 126, "GPU TEMP", 9, MUTED)
    text(d, 120, 126, "83C" if too_hot else "52C", 9, INK, True)
    text(d, 19, 147, "BATTERY", 9, MUTED)
    text(d, 120, 147, "N/A", 9, INK, True)
    text(d, 19, 171, "CAT / HOT" if too_hot else "CAT / HEALTHY", 8, TEAL)
    text(d, 19, 182, "SIMULATED / NOT REAL", 7, MUTED)
    return pic


def trends():
    # All plotted measurements in the preview are synthetic and labeled DEMO.
    pic, d = page("PC TRENDS", "music.png", 11, "DEMO")
    text(d, 19, 84, "5 MIN / CPU GPU RAM", 8, TEAL)
    for grid in range(3):
        y = 103 + grid * 65 // 2
        d.line(((19*S, y*S), (154*S, y*S)), fill=TRACK, width=2*S)
    for k, color in enumerate((TEAL, CORAL, (143, 110, 208))):
        pts = []
        for x in range(135):
            value = (48 + 24 * math.sin(x/13 + k*0.8)
                     + (8 if k == 1 else 0)) if k != 2 else 65+5*math.sin(x/29)
            pts.append(((19+x)*S, int(168 - value*65/100)*S))
        d.line(pts, fill=color, width=2*S, joint="curve")
    text(d, 19, 178, "CPU", 8, TEAL)
    text(d, 69, 178, "GPU", 8, CORAL)
    text(d, 121, 178, "RAM", 8, (143, 110, 208))
    return pic


def focus():
    pic,d = page("FOCUS TIMER", "paused.png", 2, "LIVE")
    text(d, 19, 87, "STAY IN YOUR ZONE", 8, MUTED, True)
    text(d, 16, 105, "24:59", 31, INK, True)
    bar(d,19,143,137,9,0.23)
    text(d, 19, 168, "f   play / pause", 9, MUTED)
    return pic


def network():
    pic,d = page("WI-FI STATUS", "hotspot.png", 3, "DEMO")
    text(d, 19, 87, "NOT CONNECTED", 9, CORAL, True)
    text(d, 19, 107, "7", 37, INK, True)
    text(d, 19, 157, "networks near you", 9, MUTED)
    text(d, 19, 175, "r   scan again", 9, TEAL)
    return pic


def printer():
    pic,d = page("3D PRINTING", "bambu.png", 6)
    center = (90, 128)
    r = 39
    for a in range(-90,271,5):
        rad = math.radians(a)
        x = int((center[0] + r*math.cos(rad))*S)
        y = int((center[1] + r*math.sin(rad))*S)
        d.ellipse((x-9,y-9,x+9,y+9),fill=TRACK if a > 144 else TEAL)
    text(d, 63, 114, "65", 27, INK, True)
    text(d, 117, 127, "%", 9, MUTED)
    text(d, 21, 178, "205C NOZZLE | MOCK", 8, MUTED)
    return pic


def main():
    frames = [pc_monitor(), health(), trends(), network()]
    margin = 28*S
    board = Image.new("RGB", (2*W*S+3*margin,2*H*S+3*margin),
                      (229,225,221))
    for index, frame in enumerate(frames):
        x = margin + (index % 2)*(W*S+margin)
        y = margin + (index // 2)*(H*S+margin)
        board.paste(frame.convert("RGB"),(x,y))
    board.save(ROOT / "preview.png", optimize=True)
    print("Rendered preview.png")


if __name__ == "__main__":
    main()
