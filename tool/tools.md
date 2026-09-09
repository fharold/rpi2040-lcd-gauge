## pilo

### usage: pilo VARIANT
#### example: pilo oil_p
#### flashes a build variant onto an RP2040 in BOOTSEL mode

- `pilo` without argument lists the three variants and where their `.uf2` was found
- variants: `oil_t` (engine oil temp), `gearbox_t` (gearbox oil temp), `oil_p` (engine oil pressure)
- looks in `build/`; warns if `main.c` / `CMakeLists.txt` is newer than the `.uf2`
- an explicit file also works: `pilo build/main_oil_p.uf2`
- `sudo` is used on Linux only; override with `PILO_SUDO=0` / `PILO_SUDO=1`
- with several boards in BOOTSEL, pick one: `PILO_BUS=1 PILO_ADDRESS=5 pilo oil_p`
  (find the values with `picotool info -a`)

To get the board into BOOTSEL: hold BOOT while plugging it in, or double-tap
reset - the build links `pico_bootsel_via_double_reset`.


## pirel

### usage: pirel TAG [gh release create args...]
#### example: pirel v1.0.0
#### builds the three variants and publishes them as a GitHub release

- requires the GitHub CLI (`gh`) and `PICO_SDK_PATH`
- refuses to run on a dirty working tree
- extra arguments go to `gh release create`: `pirel v1.0.0-rc1 --prerelease`
- for official releases prefer pushing a `v*` tag and letting
  `.github/workflows/release.yml` build it, so the build does not depend on
  one machine


## pigsh

### usage: pigsh ttyACMn
#### example: pigsh 0
#### connect to your device, send commands, make screenshots


## snaps
### usage: snaps ACMNUMBER FILENAME
### example: snaps 0 mysnapshot
- starts a snapshot reading data from /dev/ttyACM0 [ACMNUMBER]
- the 'bmp' image will be converted (and flipped) to 'png'
- your output file name will be: 'mysnapshot.png' according to the above example
- if it hangs reset your device and repeat the process

snaps will call mksnap ACMNUMBER FILENAME
sleeps 2 seconds
then sends " " then "SNAPSHOT" to /dev/ttyACM[ACMNUMBER]
mksnap will open the 'png' with eog image viewer (if installed)

## snaps via ssh?
### example:
### ssh user@host 'cd universal_gauges/img; ../tool/snaps 1 mysnap'
### universal_gauges has to be in your home '/home/user/universal_gauges'
- connects to your host (like jim@raspi4 )
- changes to your 'universal_gauges/img' folder in your home (so images saved are there)
- calls 'snaps' script from 'tools' folder
- connection finished - ssh stuff done
##

### local image view: [you work from a linux system, ssh connected to a pi]
### ssh user@host 'cd universal_gauges/img; ../tool/snaps 1 mysnap'; eog mysnap.png
- assumptions:
- 'sshfs user@host:/home/user/universal_gauges' ./universal_gauges
- cd ~/universal_gauges/img
- my pi4 is headless, so no X installed.

### from your local terminal:
- ssh user@host 'cd universal_gauges/img; ../tool/snaps 1 mysnap'; eog mysnap.png



## obsolete [use snaps]
### snapshot

i left the old code if you want to play

### usage: snapshot CUTFILE
#### example: snapshot s_cn
#### converts data from device to image

### connect the RP2040-LCD-1.28 via USB to your Pi

#### in a terminal:

1. pigsh 0
2. press return once (the first input is ignored)
3. type 'SNAPSHOT' and press enter
4. in a 2nd terminal: 'cat /dev/ttyACM0 > snapshotcut'
    - the pointer should be 'frozen' now on the clock, data is transmitted
    - when the pointer starts moving again press 'control+c'
5. snapshot snapshotcut
6. hopefully creates snapshotcut.bmp
## obsolete


## img2data

### usage: img2data IMAGEFILE SIZE*SIZE
#### example: img2data us16.png 16*16
#### generates file 'us16.h' with rgb565 data

#### you can add it to your .bashrc or whatever you use

function img2data()
{
oname=$(echo "$1" | cut -f 1 -d '.')
name=$(echo "$1" | cut -f 1 -d '.')rgb565
echo "name: $name"
convert $1 -rotate 180 -flop -alpha off -define bmp:subtype=RGB565 $name.bmp
#conversion: swap bytes (hi/lo)
dd conv=swab bs=138 skip=1 if=$name.bmp of=$oname.raw
touch $oname.h
echo "const unsigned char $oname[$2*2] = {" > $oname.h
hexdump -v -e '"0x" 1/1 "%02X" ","' $oname.raw >> $oname.h
rm $oname.raw
rm $name.bmp
echo "};" >> $oname.h
}

## earth / eye texture creation

- create earth.png with 256*256 size

- (sc)roll image to center:
`convert earth.png -roll +128+128 e8.png`

- this process is reversible:
`convert e8.png -roll +128+128 earth256.png`
 creates the 'regular' earth image

- now we need the image data as header file:
`img2data e8.png 256*256`

e8.h is ready to be included.


## mkfontimagexy

### usage: mkfontimagexy FONT X Y POINTSIZE
#### example: mkfontimagexy nf.ttc 31 40 27;
#### generates file 'nf.tcc.png' (2color)

#### POINTSIZE is responsible for X/Y of fonts so you have 'play' with the values a bit

A complete conversion process would be:
- install fonts
`cp /usr/share/fonts/opentype/noto/NotoSansCJK-Thin.ttc ./nf.ttc`
`mkfontimagexy nf.ttc 31 40 27`
`eog nf.ttc.png`

This generates 'nf.ttc.h' (aka Font40.h) and Font30.h is made alike.
Those fonts contain Latin, Cyrillic and a bunch of 'Chinese' characters – to write the weekdays.
A bit of show - generally it's NOT a good idea mixing fonts of different dimensions (rect vs. square)
or better: square-rect == waste-o-space

function mkfontimagexy()
{
convert -size $2x$((235*$3)) xc:black -font "$1" -pointsize $4 -fill white -draw "@b.txt" -threshold 0% -depth 1 -flip $1.png
convert $1.png -depth 1 $1.bmp
dd bs=150 skip=1 if=$1.bmp of=$1.raw
xxd -i $1.raw $1.h
rm $1.bmp
rm $1.raw
sed -i -r 's/\w+\[\]/Font'$3'_table\[\]/g' $1.h
sed -i -r 's/\w+_len/Font'$3'_len/g' $1.h
echo -e "\nsFONT Font"$3" = {\n  Font"$3"_table,\n  "$2", /* Width */\n  "$3", /* Height */\n};\n" >> $1.h
}

## New font functions

### Generate the textfile containing the draw information for 'imagemagick'
### eg: 'mkfontmt asian.txt'
### The glyphs string contains the sorted (utf8 to 4byte) glyphs (characters)
### (the sorting 1,3,2,.... in 'chinese' is correct because in 4byte '三' comes before '二')
### In the code search for 'lcd_makeutf8table(' in 'main('
### Where every 'word' is added to one big string.
### This string is striped of dupes and finally sorted to build an index table.
### Check the debug output for the complete, sorted string. 
### IF you introduce new glyphs/characters (a word with a character that has not yet occured)
### you also have to generate new fonts containing these glyphs/characters

function mkfontmt()
{
glyphs="一三二五六四土日星曜月期木水火金금목수요월일토화"
for (( i=0; i<${#glyphs}; i++ )); do
  echo "text 0,$((($i+1)*$1)) '${glyphs:$i:1}'" >> $2
done
}


function mkfontm32()
{
NUMCHARS=24
echo $NUMCHARS
echo "NUMCHARS: $NUMCHARS"
convert -size $3x$(($NUMCHARS*$4)) xc:black -fill white -stroke '#3333aa' -strokewidth 1 -font "$1" -density 72 -pointsize $2 -draw "@multi_asian.txt" -roll +1-8 -crop +0+0 -crop +0-0 -flip -colorspace RGB png24:$5.png
convert $5.png -define bmp:subtype=RGB565 $5.bmp
dd conv=swab bs=138 skip=1 if=$5.bmp of=$5.raw
xxd -i $5.raw $5.h
rm $5.bmp
rm $5.raw
sed -i -r 's/unsigned char/const char/' $5.h
sed -i -r 's/\w+\[\]/fa'$4'_table\[\]/g' $5.h
sed -i -r 's/\w+_len/fa'$4'_len/g' $5.h
echo -e "\nfont_t fa"$4" = {\n  fa"$4"_table,\n  "$3", /* Width */\n  "$4", /* Height */\n "$NUMCHARS", /* number of chars */\n  "$(((NUMCHARS-1)*$3))" /* modulo */\n};\n" >> $5.h
}

function mkfontm()
{
NUMCHARS=24
echo "NUMCHARS: $NUMCHARS"
convert -size $3x$(($NUMCHARS*$4)) xc:black -fill white -stroke '#3333aa' -strokewidth 1 -font "$1" -density 72 -pointsize $2 -draw "@multi_a40.txt" -roll +2-8 -crop +0+0 -crop +0-0 -flip -colorspace RGB png24:$5.png
convert $5.png -define bmp:subtype=RGB565 $5.bmp
dd conv=swab bs=138 skip=1 if=$5.bmp of=$5.raw
xxd -i $5.raw $5.h
rm $5.bmp
rm $5.raw
sed -i -r 's/unsigned char/const char/' $5.h
sed -i -r 's/\w+\[\]/fa'$4'_table\[\]/g' $5.h
sed -i -r 's/\w+_len/fa'$4'_len/g' $5.h
echo -e "\nfont_t fa"$4" = {\n  fa"$4"_table,\n  "$3", /* Width */\n  "$4", /* Height */\n "$NUMCHARS", /* number of chars */\n  "$(((NUMCHARS-1)*$3))" /* modulo */\n};\n" >> $5.h
}


function mkfont_asian()
{
mkfontm32 ./NotoSansCJK-Bold.ttc 20 24 32 fa32; eog fa32.png;feh fa32.png
mkfontm ./NotoSansCJK-Bold.ttc 25 30 40 fa40; eog fa40.png;feh fa40.png
}

function mkfont_kyrillic()
{
mkfontkyr32 ./OpenSans-Regular.ttf 28 22 32 fkyr32;eog fkyr32.png;feh fkyr32.png
mkfontkyr ./OpenSans-ExtraBold.ttf 25 24 40 fkyr40; eog fkyr40.png;feh fkyr40.png
}


function mkfontkyr()
{
NUMCHARS=18
echo "NUMCHARS: $NUMCHARS"
convert -size $3x$(($NUMCHARS*$4+8)) xc:black -fill white -kerning 1 -density 72 -font "$1" -pointsize $2 -draw "@multi_kyr40.txt" -roll +2-6 -crop +0+0 -crop +0-0 -flip -colorspace RGB png24:$5.png
convert $5.png -define bmp:subtype=RGB565 $5.bmp
dd conv=swab bs=138 skip=1 if=$5.bmp of=$5.raw
xxd -i $5.raw $5.h
rm $5.bmp
rm $5.raw
sed -i -r 's/unsigned char/const char/' $5.h
sed -i -r 's/\w+\[\]/fkyr'$4'_table\[\]/g' $5.h
sed -i -r 's/\w+_len/fkyr'$4'_len/g' $5.h
echo -e "\nfont_t fkyr"$4" = {\n  fkyr"$4"_table,\n  "$3", /* Width */\n  "$4", /* Height */\n "$NUMCHARS", /* number of chars */\n  "$(((NUMCHARS-1)*$3))" /* modulo */\n};\n" >> $5.h
}

function mkfontkyr32()
{
NUMCHARS=$(stat --printf="%s" multi_kyr.txt)
echo $NUMCHARS
NUMCHARS=18
W=$4
echo "NUMCHARS: $NUMCHARS"
convert -size $3x$(($NUMCHARS*$4+4)) xc:black -fill white -kerning 1 -density 72 -font "$1" -pointsize $2 -draw "@multi_kyr.txt" -roll +1-4 -crop +0+0 -crop +0-0 -flip -colorspace RGB png24:$5.png
convert $5.png -define bmp:subtype=RGB565 $5.bmp   
dd conv=swab bs=138 skip=1 if=$5.bmp of=$5.raw
xxd -i $5.raw $5.h
rm $5.bmp
rm $5.raw
sed -i -r 's/unsigned char/const char/' $5.h
sed -i -r 's/\w+\[\]/fkyr'$W'_table\[\]/g' $5.h
sed -i -r 's/\w+_len/fkyr'$W'_len/g' $5.h
echo -e "\nfont_t fkyr"$W" = {\n  fkyr"$W"_table,\n  "$3", /* Width */\n  "$W", /* Height */\n "$NUMCHARS", /* number of chars */\n  "$(((NUMCHARS-1)*$3))" /* modulo */\n};\n" >> $5.h
}
