cp ./.pio/build/adafruit_qt_py_m0/firmware.bin ./f128-hdr-86mm.bin
python3 ./uf2conv.py -o ./f128-hdr-86mm.uf2 ./f128-hdr-86mm.bin