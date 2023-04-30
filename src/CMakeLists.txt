set(EXE_NAME ${PROJECT_NAME})

add_executable(${EXE_NAME})

target_sources(${EXE_NAME}
    PRIVATE
    "main.c"
)

# libraries have to be preinstalled, 
# or be available through a toolchain-file
find_package(libmodbus REQUIRED)
find_package(Threads REQUIRED)

target_link_libraries(${EXE_NAME}
    libmodbus::libmodbus
    Threads::Threads
)

install_targets(
    TARGETS ${EXE_NAME}
)

