convert https://github.com/Dreamacro/clash/raw/master/docs/logo.png -set option:modulate:colorspace hsl -modulate 120,200,56 +repage logo.png
convert logo.png -define trim:minSize=360x360 -gravity center -background rgba(0,0,0,0) -trim trimmed.png

convert trimmed.png -resize 16x16 -depth 32 16-32.png
convert trimmed.png -resize 32x32 -depth 32 32-32.png
convert trimmed.png -resize 48x48 -depth 32 48-32.png

convert 16-32.png 32-32.png 48-32.png icon.ico