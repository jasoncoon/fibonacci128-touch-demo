cp ./.pio/build/adafruit_qt_py_m0/firmware.bin ./1f128.bin
python3 ./uf2conv.py -o ./1f128.uf2 ./1f128.bin