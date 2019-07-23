#include <cstdio>
#include "test_service.h"

int main(int, char *[])
{
    bool use_tcp = true;
    bool requester = true;
    std::size_t send_times = 0;
    std::size_t connection_count = 100000;

    TestService client(use_tcp, requester, send_times, connection_count);

    client.init();

    getchar();

    client.exit();

    return (0);
}
