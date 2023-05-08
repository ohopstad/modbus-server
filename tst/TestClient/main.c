#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<modbus/modbus.h>

enum ConnectionTypes{
    RTU, // TODO
    TCP, TCP_PI
};

struct Options
{
    enum ConnectionTypes Type;
    char* Address;
    char Port[6]; 
};

struct Options ReadFlags(int argc, char** argv)
{
    struct Options ret = { // Defaults
        .Type = TCP,
        .Address = "127.0.0.1",
        .Port = "1502"
    };
    // TODO: Handle CLI args
    return ret;
};

int main(int argc, char** argv)
{
    struct Options opt = ReadFlags(argc, argv);

    modbus_t* client = NULL;
    int sock = -1;
    int status = -1;

    switch(opt.Type)
    {
    case TCP:
        client = modbus_new_tcp(opt.Address, atoi(opt.Port));
        break;
    case TCP_PI:
        client = modbus_new_tcp_pi(opt.Address, opt.Port);
    default:
        return 1;
    }

    status = modbus_connect(client);

    if (status == -1) // Failed to connet to server
    {
        fprintf(stderr, "Failed to connect to server %s:%s\n", opt.Address, opt.Port);
        modbus_free(client);
        return 1;
    }

    printf("setup finished.\ninput: [r/w reg# val] (val will be ignored if r is entered)\n");
    while(1)
    {
        char read_write = '\0';
        int reg_num = 0;
        int value = 0;


        scanf("%c %d %d", &read_write, &reg_num, &value);
        
        if(read_write == 'r')
        {
            status = modbus_read_registers(client, reg_num, 1, (uint16_t*) &value);
            if (status != 1)
            {
                fprintf(stderr, "ERROR %d reading register: %s\n", errno, modbus_strerror(errno));
                continue;
            }
            printf("\t %d\n", value);
        } 
        else if (read_write == 'w')
        {
            status = modbus_write_registers(client, reg_num, 1, (uint16_t*) &value);
            if (status != 1)
            {
                fprintf(stderr, "ERROR %d reading register: %s\n", errno, modbus_strerror(errno));
                continue;
            }
        }
    }
}
