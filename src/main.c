// std
#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<errno.h>
#include<netdb.h>
#ifdef _WIN32
    #include<winsock.h>
#else
    #include<sys/socket.h>
#endif
// ext
#include<modbus/modbus.h>

static pthread_mutex_t db_access = PTHREAD_MUTEX_INITIALIZER;
static modbus_mapping_t* db = NULL;

enum ctx_type
{
    RTU, // TODO 
    TCP, TCP_PI
};

struct options
{
    enum ctx_type type;
    char port[6];
    int nb_clients;
    int nb_bits;
    int nb_wbits;
    int nb_regs;
    int nb_wregs;
};

struct options ReadFlags(int argc, char ** argv)
{
    struct options ret = { // defaults
        .type = TCP,
        .port = "1502",
        .nb_clients = 10,
        .nb_bits = MODBUS_MAX_READ_BITS,
        .nb_wbits = MODBUS_MAX_WRITE_BITS,
        .nb_regs = MODBUS_MAX_READ_REGISTERS,
        .nb_wregs = MODBUS_MAX_WRITE_REGISTERS
    };
    // TODO

    return ret;
};

int FindClientName(int socket, char* name, int name_len)
{
    struct sockaddr_storage addr;
#ifdef _WIN32
    int addr_len = 0;
#else 
    socklen_t addr_len = 0;
#endif

    if (getpeername(socket, (struct sockaddr*)&addr, &addr_len) != 0)
    {
        // TODO: some errorhandling
    }
    if (getnameinfo((struct sockaddr*)&addr, addr_len, name, name_len, NULL, 0, NI_NUMERICHOST) != 0)
    {
        // TODO: more error handling
    }
    return 0;
}


// should handle the connection of one client in its own thread
void* HandleConnection(void* arg)
{
    modbus_t* client = (modbus_t*) arg;

    char clientName[NI_MAXHOST];
    FindClientName(modbus_get_socket(client), clientName, NI_MAXHOST);
    printf("new connection from: %s\n", clientName);

    while (1)
    {
        uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];
        int rx_count = -1;
        int status = -1;

        rx_count = modbus_receive(client, query);
        if (rx_count > 0)
        {
            size_t queryString_len = (MODBUS_TCP_MAX_ADU_LENGTH *3);
            char queryString[queryString_len];

            for (int c = 0; c < rx_count ; c++)
            {
                sprintf(queryString, "%x ", query[c]);
            }
            printf("Query from %s:\n\t[%s]\n", clientName, queryString);
            pthread_mutex_lock(&db_access);
            status = modbus_reply(client, query, rx_count, db);
            pthread_mutex_unlock(&db_access);
            if (status != 0){
                fprintf(stderr, "error %d replying to query from %s: %s\n", errno, clientName, modbus_strerror(errno));
            }
        } else if (rx_count < 0){
            // error or closed
            printf("%s disconnected\n", clientName);
            return NULL; 
        } else {
            // empty query
        }
    }
};

int main(int argc, char** argv)
{
    struct options opt = ReadFlags(argc, argv);

    modbus_t* server = NULL;
    int sock = -1;
    int status = -1;

    switch (opt.type)
    {
    case TCP :
        server = modbus_new_tcp(NULL, atoi(opt.port));
        sock = modbus_tcp_listen(server, 1);
        break;
    case TCP_PI :
        server = modbus_new_tcp_pi(NULL, opt.port);
        sock = modbus_tcp_pi_listen(server, 1);
        break;
    default : // TODO: RTU
        return 1;
    }

    db = modbus_mapping_new(
        opt.nb_bits, opt.nb_wbits
        , opt.nb_regs, opt.nb_wregs
    );    

    printf("Setup done.\n");
    while(1)
    {
        int client_sock = -1;
        modbus_t* client = NULL;
        pthread_t* thread = NULL;

        switch (opt.type){
        case TCP :
            client_sock = modbus_tcp_accept(server, &sock);
            client = modbus_new_tcp(NULL, atoi(opt.port));
            break;
        case TCP_PI :
            client_sock = modbus_tcp_pi_accept(server, &sock);
            client = modbus_new_tcp_pi(NULL, opt.port);
            break;
        default :
            fprintf(stderr, "RTU not implemented.\n");
            return 1;
        }
        status = modbus_set_socket(client, client_sock);
        if (status == -1)
        {
            fprintf(stderr, "%d: Something went wrong! -> %s\n", errno, modbus_strerror(errno));
        }
        pthread_create(thread, NULL, HandleConnection, (void*) client); // casting to void* is dangerous
        pthread_detach(*thread); // I hope the thread is killed automatically if this "main" thread is killed...
    }
};
