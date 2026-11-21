"""TFT design proof using the generated firmware bitmaps (not a device capture).

No font downloads or installed fonts are needed. Device SPI/colors and the
LovyanGFX implementation remain authoritative; this is a layout simulation.
"""
from pathlib import Path
import re
from PIL import Image, ImageDraw

ROOT=Path(__file__).resolve().parents[1]


class BitmapFont:
    def __init__(self, name):
        source=(ROOT/'src/render/music_fonts.h').read_text(encoding='utf-8')
        block=lambda kind: re.search(name+kind+r'\[\].*?=\s*\{(.*?)\};',source,re.S)[1]
        self.bitmap=bytes(int(x,16) for x in re.findall(r'0x([0-9a-f]{2})',block('Bitmap')))
        self.glyphs=[tuple(map(int,x.split(','))) for x in re.findall(r'\{([^{}]+)\}',block('Glyphs'))]
        self.ranges=[tuple(map(int,x.split(','))) for x in re.findall(r'\{([^{}]+)\}',block('Ranges'))]
        self.advance=27 if name=='title' else 21

    def glyph(self,ch):
        for start,end,base in self.ranges:
            if start<=ord(ch)<=end:
                return self.glyphs[base+ord(ch)-start]
        return self.glyph('?')

    def width(self,value):
        return sum(self.glyph(c)[3] for c in value)

    def draw(self,image,value,x,baseline,color):
        for ch in value:
            offset,w,h,advance,dx,dy=self.glyph(ch)
            for y in range(h):
                for col in range(w):
                    bit=y*w+col
                    if self.bitmap[offset+bit//8] & (0x80>>(bit%8)):
                        px,py=x+dx+col,baseline+dy+y
                        if 0<=px<image.width and 0<=py<image.height: image.putpixel((px,py),color)
            x+=advance

    def lines(self,image,value,x,baseline,width,limit,color):
        start=0
        for row in range(limit):
            if start>=len(value): break
            end=space=start
            for next in range(start+1,len(value)+1):
                candidate=value[start:next]+('…' if row==limit-1 and next<len(value) else '')
                if self.width(candidate)>width: break
                end=next
                if value[next-1]==' ': space=end
            if row<limit-1 and end<len(value) and space>start: end=space
            line=value[start:end].strip()+('…' if row==limit-1 and end<len(value) else '')
            self.draw(image,line,x,baseline+row*self.advance,color)
            if end==start: break
            start=end
            while start<len(value) and value[start]==' ': start+=1


def render(title,artist,dark=False,playing=True):
    body,heading=BitmapFont('body'),BitmapFont('title')
    bg,paper,ink,muted,mint,teal=(('#1e222c','#2d3241','#f8f4ef','#c8cbd0','#2e5247','#9be8c6') if dark else
                               ('#fff8f1','#ffffff','#2b3145','#5d6170','#bdeedc','#358e7b'))
    rgb=lambda c: tuple(bytes.fromhex(c[1:]))
    bg,paper,ink,muted,mint,teal=map(rgb,(bg,paper,ink,muted,mint,teal))
    image=Image.new('RGB',(320,240),bg);d=ImageDraw.Draw(image)
    d.rounded_rectangle((9,7,234,36),15,fill=paper)
    heading.draw(image,'mimi.',21,29,teal);body.draw(image,'MUSIC / DEMO',100,27,muted)
    d.rounded_rectangle((8,43,311,202),12,fill=paper)
    d.rounded_rectangle((16,49,135,168),9,fill=mint)
    for r in (35,25): d.ellipse((76-r,109-r,76+r,109+r),outline=teal,width=1)
    d.ellipse((69,102,83,116),fill=teal)
    heading.lines(image,title,148,67,156,3,ink)
    body.lines(image,artist,148,145,156,2,muted)
    d.rounded_rectangle((16,176,303,180),2,fill=mint)
    d.rounded_rectangle((16,176,132,180),2,fill=teal)
    body.draw(image,'1:32 / 3:47',16,199,muted)
    body.draw(image,'Đang phát' if playing else 'Tạm dừng',214 if playing else 210,197,teal)
    for x,caption in ((11,'Home'),(85,'PC'),(159,'Music'),(233,'Focus')):
        d.rounded_rectangle((x,211,x+70,231),9,fill=mint if caption=='Music' else paper)
        body.draw(image,caption,x+12,226,teal if caption=='Music' else muted)
    return image


if __name__=='__main__':
    target=ROOT.parent/'docs/music-v210-tft-preview.png'
    result=Image.new('RGB',(640,240),'#fff8f1')
    result.paste(render('Đừng làm trái tim anh đau','Sơn Tùng M-TP'),(0,0))
    result.paste(render('Những giấc mơ rực rỡ giữa lòng thành phố','Đỗ Mỹ Linh · Mỹ Tâm',True,False),(320,0))
    result.resize((1280,480),Image.Resampling.NEAREST).save(target)
    print(f'TFT SIMULATION (not hardware): {target}')
