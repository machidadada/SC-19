# ビルドを実行するファイルを追加
add_library(SC STATIC
    ${CMAKE_CURRENT_LIST_DIR}/sc.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sc_pico.cpp
)
# 以下の資料を参考にしました
# https://qiita.com/kikochan/items/732e46e92e7f29c18ce9
# https://qiita.com/shohirose/items/45fb49c6b429e8b204ac

# ライブラリの読み込み
target_link_libraries(SC
    hardware_gpio
    hardware_i2c
    hardware_pwm
    hardware_spi
    hardware_uart
    pico_stdlib
)

##################################################
##################################################
##################################################

# # 最低限必要なCMakeのバージョンを設定
# cmake_minimum_required(VERSION 3.12)

# # プログラミング言語を設定
# set(CMAKE_C_STANDARD 11)
# set(CMAKE_CXX_STANDARD 17)

# # SDKの読み込み (プロジェクトに関する設定の前にある必要がある)
# include(pico_sdk_import.cmake)

# # プロジェクト名
# project(SC C CXX ASM)

# # 例外を有効にする
# set(PICO_CXX_ENABLE_EXCEPTIONS 1)
# set(PICO_CXX_ENABLE_RTTI 1)
# # 以下の資料を参考にしました
# # https://community.element14.com/products/raspberry-pi/b/blog/posts/raspberry-pico-and-cmake---enable-c-exceptions-and-rtti-run-time-type-information

# # 警告レベルを上げる
# if(MSVC)
#     set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /EHsc")
# else()
#     set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++17")
# endif()

# # SDKを初期化
# pico_sdk_init()

# # ビルドを実行するファイルを追加
# add_executable(SC
#     sc.cpp
#     sc_test.cpp
# )

# # ライブラリの読み込み
# target_link_libraries(SC
#     pico_stdlib
#     hardware_gpio
#     hardware_i2c
#     hardware_spi
#     hardware_uart
#     hardware_pwm
# )

# # USB出力を有効にし，UART出力を無効にする
# pico_enable_stdio_usb(SC 1)
# pico_enable_stdio_uart(SC 0)

# # map/bin/hex/uf2などのファイルを追加で出力する
# pico_add_extra_outputs(SC)