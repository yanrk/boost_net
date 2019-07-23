#include <cstdio>
#include "test_service.h"

int main(int, char *[])
{
    bool use_tcp = true;
    bool requester = false;
    std::size_t send_times = 0;
    std::size_t connection_count = 100000;

    TestService server(use_tcp, requester, send_times, connection_count);

    server.init();

    getchar();

    server.exit();

    return (0);
}
