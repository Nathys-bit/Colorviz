# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(Colorviz C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(Colorviz Colorviz.c 
    tcs34725.c 
    identificador_cor.c 
    filtros_daltonismo.c
    inc/ssd1306_i2c.c
    config.c
    dhcpserver/dhcpserver.c
    dnsserver/dnsserver.c
    core1.c
    )

pico_set_program_name(Colorviz "Colorviz")
pico_set_program_version(Colorviz "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(Colorviz 0)
pico_enable_stdio_usb(Colorviz 1)
# Add the standard library to the build
target_link_libraries(Colorviz
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(Colorviz PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/inc
        ${CMAKE_CURRENT_LIST_DIR}/dhcpserver
        ${CMAKE_CURRENT_LIST_DIR}/dnsserver
        ${CMAKE_CURRENT_LIST_DIR}/.. 
)

# Add any user requested libraries
target_link_libraries(Colorviz 
        hardware_i2c
        hardware_adc
        m
        pico_sync
        pico_multicore 
        pico_cyw43_arch_lwip_poll

        )

pico_configure_ip4_address(Colorviz PRIVATE
        CYW43_DEFAULT_IP_AP_ADDRESS 192.168.4.1
        )

pico_add_extra_outputs(Colorviz)

