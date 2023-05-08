// std
#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<errno.h>
#include<netdb.h>
#include<string.h>
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
    size_t max_clients;
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
        .nb_wregs = MODBUS_MAX_WRITE_REGISTERS,
        .max_clients = 5
    };
    // TODO

    return ret;
};

int FindClientName(int socket, char* name, int name_len)
{
    struct sockaddr_storage addr;
#ifdef _WIN32
    int addr_len = sizeof(addr);
#else 
    socklen_t addr_len = sizeof(addr);
#endif
    int status = -1;

    status = getpeername(socket, (struct sockaddr*)&addr, &addr_len); 
    if (status != 0)
    {
        // TODO: some errorhandling
        fprintf(stderr, "getpeername error\n\t%s\n", gai_strerror(status));
        return -1;
    }

    status = getnameinfo((struct sockaddr*)&addr, addr_len, name, name_len, NULL, 0, NI_NUMERICHOST);
    if (status != 0)
    {
        // TODO: more error handling
        fprintf(stderr, "getnameinfo error\n\t%s\n", gai_strerror(status));
        return -1;
    }
    return 0;
}


// should handle the connection of one client in its own thread
void* HandleConnection(void* arg)
{
    modbus_t* client = arg;
    int status = -1;

    char clientName[NI_MAXHOST];
    status = FindClientName(modbus_get_socket(client), clientName, NI_MAXHOST);
    if (status != 0)
    {
        fprintf(stderr, "error fetching name\n");
        return NULL;
    }
    printf("new connection from: %s\n", clientName);

    while (1)
    {
        uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];
        int rx_count = -1;
        int status = -1;

        rx_count = modbus_receive(client, query);
        if (rx_count > 0)
        {
            char queryString[MODBUS_TCP_MAX_ADU_LENGTH *3 +1];
            memset(queryString, 0, sizeof(queryString) -1);

            for (int c = 0; c < rx_count ; c++)
            {
                char tmp[4];
                sprintf(tmp, "%x ", query[c]);
                strcat(queryString, tmp);
            }
            printf("Query from %s: [%s]\n", clientName, queryString);

            pthread_mutex_lock(&db_access);
            status = modbus_reply(client, query, rx_count, db);
            pthread_mutex_unlock(&db_access);
            if (status == -1){
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
    pthread_t clients[opt.max_clients];

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

    for(int c = 0; c < opt.max_clients; c++)
    {
        clients[c] = NULL;
    }


    printf("Setup done.\n");
    while(1)
    {
        int client_sock = -1;
        modbus_t* client = NULL;

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

        for(size_t c = 0; c < opt.max_clients; c++)
        {
            // find a null or finished client thread
            if (
                clients[c] != NULL 
                & pthread_kill(clients[c], 0) != 0
            ){
                if (c == opt.max_clients -1)
                {
                    fprintf(stderr, "No available client threads.\n");
                    // TODO: some handling of the client that will now have to be disconnected.
                }
                continue;
            }
            // casting to void* is dangerous ?
            pthread_create(&clients[c], NULL, &HandleConnection, (void*) client); 
            break;
        }
    }
};
