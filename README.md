# modbus-server
A small server for modbus clients based on https://github.com/stephane/libmodbus 


## Build
(from /build/ directory)
```sh
gcc ../src/main.c -o Server -L/path/to/libmodbus/lib -I/path/to/libmodbus/include -lpthread -lmodbus
```
