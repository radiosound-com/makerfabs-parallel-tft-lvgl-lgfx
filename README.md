# LVGL + LGFX for MakerFabs 3.5" Parallel TFT ESP32-S2 and S3

A simple project that puts all the LVGL demos together in esp-idf with the LovyanGFX driver

## Credits

* sukesh-ak's WT32-SC01 project: https://github.com/sukesh-ak/LVGL8-WT32-SC01-IDF
* LovyanGFX's panel/touch/light definitions for the MakerFabs S2 screen: https://github.com/lovyan03/LovyanGFX/blob/master/src/lgfx_user/LGFX_ESP32S2_MakerabsParallelTFTwithTouch.hpp
* MakerFabs' definitions for the S3 screen: https://github.com/Makerfabs/Makerfabs-ESP32-S3-Parallel-TFT-with-Touch/blob/main/firmware/SD16_3.5/SD16_3.5.ino

## Setup

Make sure you have esp-idf exported, e.g.:

```sh
source ~/esp/esp-idf/export.sh
```

Depending on which board you have:

```sh
idf.py set-target esp32s2
```

or

```sh
idf.py set-target esp32s3
```

Then

```sh
idf.py build flash monitor
```
