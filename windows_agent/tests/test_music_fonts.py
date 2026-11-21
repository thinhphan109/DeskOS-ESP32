"""Check actual embedded glyphs, including all Vietnamese vowel/tone pairs."""
import importlib.util
from pathlib import Path
import unicodedata
import unittest

ROOT=Path(__file__).resolve().parents[2]


class MusicFontTests(unittest.TestCase):
    def test_all_nfc_vietnamese_glyphs_have_real_bitmaps(self):
        spec=importlib.util.spec_from_file_location('music_preview',ROOT/'firmware/tools/render_music_preview.py')
        module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
        characters=set('Đđ')
        for vowel in 'AaĂăÂâEeÊêIiOoÔôƠơUuƯưYy':
            for tone in ('','\u0300','\u0301','\u0303','\u0309','\u0323'):
                characters.update(unicodedata.normalize('NFC',vowel+tone))
        for name in ('body','title'):
            font=module.BitmapFont(name)
            for ch in characters:
                with self.subTest(font=name,character=ch):
                    self.assertTrue(any(a<=ord(ch)<=b for a,b,_ in font.ranges))
                    offset,w,h,*_=font.glyph(ch)
                    self.assertGreater(w*h,0)
                    self.assertTrue(any(font.bitmap[offset:offset+(w*h+7)//8]))
