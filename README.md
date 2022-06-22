# LVGL + LGFX for MakerFabs 3.5" Parallel TFT ESP32-S2 and S3

A simple project that puts all the LVGL demos together in esp-idf with the LovyanGFX driver

## Credits

sukesh-ak's WT32-SC01 project: https://github.com/sukesh-ak/LVGL8-WT32-SC01-IDF
LovyanGFX's panel/touch/light definitions for the MakerFabs S2 screen: https://github.com/lovyan03/LovyanGFX/blob/master/src/lgfx_user/LGFX_ESP32S2_MakerabsParallelTFTwithTouch.hpp
MakerFabs' definitions for the S3 screen: https://github.com/Makerfabs/Makerfabs-ESP32-S3-Parallel-TFT-with-Touch/blob/main/firmware/SD16_3.5/SD16_3.5.ino

## Setup

Make sure you have esp-idf exported, e.g.:

```sh
source ~/esp/esp-idf/export.sh
```

This repo has scripts intended to keep the different targets' build directories separate. Helpful when switching so you don't always have to rebuild.

Depending on which board you have:

```sh
source ./set-target esp32s3
```

Now, instead of using idf.py build flash monitor, use:

```sh
./build flash monitor
```

To change the configuration or do any other idf.py thing besides build:

```sh
./idf menuconfig
```
