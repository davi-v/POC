from PIL import Image
im = Image.open('horse-skel.png')
orgPixels = im.load()

#print(im.mode, im.size)

img = Image.new('RGBA', im.size)

newPixels = img.load()
w, h = img.size
cnt = 0
for i in range(w):
	for j in range(h):
		px = orgPixels[i, j]
		if px[-1] == 255:
			newPixels[i, j] = 255, 255, 255, 255
		else:
			newPixels[i, j] = 0, 0, 0, 0
img.save('horse-skel2.png')

t = Image.open('horse-skel2.png')
px = t.load()
w, h = img.size
for i in range(w):
	for j in range(h):
		print(px[i, j])