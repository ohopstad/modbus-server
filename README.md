# modbus-server
A small server for modbus clients based on https://github.com/stephane/libmodbus 


## Build
(from build/ directory)
```sh
gcc ../src/main.c -o Server -L/path/to/libmodbus/lib -I/path/to/libmodbus/include -lpthread -lmodbus
```

### Test
(from build/ directory)

- manual client
    ```sh
    gcc ../tst/TestClient/main.c -o Client -L/path/to/libmodbus/lib/ -I/path/to/libmodbus/include/ -lmodbus
    ```