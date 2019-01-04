#!/bin/bash

function show_help {
  echo -e "\nUsage: $0 [-f]\n\n\t-f\tFlash image after creating it\n\t-h?\tShow help"
}

flash=

while getopts "h?f" opt; do
    case "$opt" in
    h|\?)
        show_help
        exit 0
        ;;
    f)  flash=1
        ;;
    esac
done

# get tools depending on OS
unameOut="$(uname -s)"
case "${unameOut}" in
    Darwin*)
      MKSPIFFS="/Users/Johnny/Library/Arduino15/packages/esp8266/tools/mkspiffs/0.1.2/mkspiffs"
      ESPTOOL="esptool.py"
      PORT="/dev/cu.usb*"
      ;;
    *)
      MKSPIFFS=$(find  "/c/Users/Johnny/AppData/Local/Arduino15/packages/esp8266/tools/mkspiffs/" -name "mkspiffs.exe" | head -n 1)
      ESPTOOL="esptool"
      PORT="COM6"
esac

echo -e "Converting files in folder \"website\" to gzipped files in \"data\"\n"

CURRDIR="`realpath $(dirname \"$0\")`"
WEBSITE="$CURRDIR/website/"

cd "$CURRDIR"

# remove old files
rm -rf "$CURRDIR/data"

# change into website folder
cd "$WEBSITE"

# iterate over all files
FILES=$(find * ! -path '*/.*' -type f | sort)

echo -en "Compressing files...\t"
for i in $FILES; do
  # skip testing files
  if [[ $i == *.php ]] || [[ $i == js/* ]] || [[ $i == css/* ]]; then
    continue
  fi

  # create subfolder if necessary
  mkdir -p "$(dirname "../data/$i")"

  # gzip file
  cat $i | gzip > "../data/$i"
done

echo -en "Done.\nMinifying JS...\t\t"
cd js
./minify.command 
mkdir -p "$CURRDIR/data/js" && cat minified.js | gzip > "$CURRDIR/data/js/minified.js"
cd ..

echo -en "Done.\nMinifying CSS...\t"
cd css
./minify.command
mkdir -p "$CURRDIR/data/css" && cat minified.css | gzip > "$CURRDIR/data/css/minified.css"
cd "$CURRDIR"

exit 0

echo -en "Done.\nCreating image...\t"
# Create SPIFFS binary file: 3M
"$MKSPIFFS" -c data -p 256 -b 8192 -s $((0x3FB000 - 0x100000)) SPIFFS.bin > /dev/null

# Print size of folder
echo -e "Done.\n\nTotal file size:\t$(du -sh "$CURRDIR/data" | awk '{print $1;}')"

# Remove created files
rm -rf "$CURRDIR/data"

if [ ! -z "$flash" ]; then
  # Flash binary file
  "$ESPTOOL" --port $PORT write_flash 0x100000 SPIFFS.bin
else
  echo -e "\nImage not flashed, use -f option to flash"
fi
