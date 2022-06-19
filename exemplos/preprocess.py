IN = 't1.png'
OUT = 't1-2.png'

from PIL import Image
im = Image.open(IN)
orgPixels = im.load()

#print(im.mode, im.size)

img = Image.new('RGBA', im.size)

newPixels = img.load()
w, h = img.size
cnt = 0
for i in range(w):
	for j in range(h):
		px = orgPixels[i, j]
		if px[-1]:
			newPixels[i, j] = tuple(list(px[:3]) + [255])
		else:
			newPixels[i, j] = 0, 0, 0, 0

img.save(OUT)