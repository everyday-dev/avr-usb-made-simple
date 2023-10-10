# Set min req version of Cmake
cmake_minimum_required(VERSION 3.16)

# Set the project name
project(avr-usb-made-simple)

add_executable(flash_led flash_led.c)
add_executable(read_var read_var.c)
add_executable(interrupt interrupt.c)
target_link_libraries(flash_led usb-1.0)
target_link_libraries(read_var usb-1.0)
target_link_libraries(interrupt usb-1.0)