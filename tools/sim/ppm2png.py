#!/usr/bin/env python3
"""Convert the simulator's PPM frames to PNG (1x and 3x) and a contact sheet."""
import glob, os, sys
from PIL import Image, ImageDraw
d = sys.argv[1]
ims = []
for p in sorted(glob.glob(os.path.join(d, "*.ppm"))):
    im = Image.open(p).convert("RGB")
    base = p[:-4]
    im.save(base + ".png")
    im.resize((960, 510), Image.NEAREST).save(base + "@3x.png")
    ims.append((os.path.basename(base), im))
cols, pad = 3, 14
rows = (len(ims) + cols - 1) // cols
sheet = Image.new("RGB", (pad + cols * (640 + pad), pad + rows * (340 + 24 + pad)), (16, 17, 20))
dr = ImageDraw.Draw(sheet)
for i, (n, im) in enumerate(ims):
    r, c = divmod(i, cols)
    x, y = pad + c * (640 + pad), pad + r * (364 + pad)
    dr.text((x, y), n, fill=(238, 238, 240))
    sheet.paste(im.resize((640, 340), Image.NEAREST), (x, y + 20))
sheet.save(os.path.join(d, "contact_sheet.png"))
