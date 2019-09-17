from os.path import isdir, join
import subprocess


Import("env")

PROJ_DIR = "$PROJECT_DIR"

env.Append(
    CPPPATH = [
        join(PROJ_DIR, "components", "boards", "include"),
        join(PROJ_DIR, "components", "drivers", "lcd", "include"),
        join(PROJ_DIR, "components", "drivers", "lcd", "lcd_sipeed", "include"),
        join(PROJ_DIR, "components", "drivers", "lcd", "lcd_st7789", "include"),
        join(PROJ_DIR, "components", "drivers", "sd_card", "include"),
        join(PROJ_DIR, "components", "third_party", "oofatfs", "include"),
        join(PROJ_DIR, "components", "utils", "base64", "include"),
        join(PROJ_DIR, "components", "utils", "cJSON", "include"),
        join(PROJ_DIR, "components", "utils", "image_op", "include"),
        join(PROJ_DIR, "components", "utils", "jpeg_decode", "include"),
        join(PROJ_DIR, "components", "utils", "jpeg_encode", "include"),
        join(PROJ_DIR, "components", "utils", "list", "src"),
        join(PROJ_DIR, "src", "face_lib"),
        join(PROJ_DIR, "src", "audio"),
        join(PROJ_DIR, "src", "camera", "include"),
        join(PROJ_DIR, "src", "camera", "ov2640", "include"),
        join(PROJ_DIR, "src", "network"),
        join(PROJ_DIR, "src", "network", "http"),
        join(PROJ_DIR, "src", "network", "mqtt"),
        join(PROJ_DIR, "src", "network", "qrcode"),
        join(PROJ_DIR, "src", "network", "wifi"),
        join(PROJ_DIR, "src", "network", "wifi", "spi"),
        join(PROJ_DIR, "src", "network", "wifi", "utility"),
        join(PROJ_DIR, "src", "uart_recv"),
        join(PROJ_DIR, "src", "include"),
        join(PROJ_DIR, "include")
    ]
)

libs = [
    env.BuildLibrary(
        join("$BUILD_DIR", "drivers"),
        join(PROJ_DIR, "components", "drivers")
    ),
    env.BuildLibrary(
        join("$BUILD_DIR", "utils-base64"),
        join(PROJ_DIR, "components", "utils", "base64")
    ),
    env.BuildLibrary(
        join("$BUILD_DIR", "utils-cJSON"),
        join(PROJ_DIR, "components", "utils", "cJSON")
    ),
    env.BuildLibrary(
        join("$BUILD_DIR", "utils-image_op"),
        join(PROJ_DIR, "components", "utils", "image_op")
    ),
    env.BuildLibrary(
        join("$BUILD_DIR", "utils-jpeg_decode"),
        join(PROJ_DIR, "components", "utils", "jpeg_decode")
    ),
    env.BuildLibrary(
        join("$BUILD_DIR", "utils-jpeg_encode"),
        join(PROJ_DIR, "components", "utils", "jpeg_encode")
    ),
    env.BuildLibrary(
        join("$BUILD_DIR", "utils-list"),
        join(PROJ_DIR, "components", "utils", "list", "src")
    )
]

env.Prepend(LIBS = libs)

#exec("python .\update_build_info.py --configfile header ./global_build_info_time.h ./global_build_info_version.h")
subprocess.run(["python", join("tools", "kconfig", "update_build_info.py"), "--configfile", "header", join("include", "global_build_info_time.h"), join("include", "global_build_info_version.h")])