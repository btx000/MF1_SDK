from os.path import isdir, join

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
        join(PROJ_DIR, "components", "utils", "list", "src")
    ]
)

libs = [
    env.BuildLibrary(
        join("$BUILD_DIR", "components"),
        join(PROJ_DIR, "components")
    )
]

env.Prepend(LIBS = libs)

