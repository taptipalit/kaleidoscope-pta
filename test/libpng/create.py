from PIL import Image
for i in range(100):
    img = Image.new('RGB', (32+i,32+i), color='black')
    img.save("input_directory/empty_"+str(i)+".png")
quit()

