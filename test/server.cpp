#include <cstdio>
#include "test_service.h"

int main(int, char *[])
{
    bool use_tcp = true;
    std::size_t send_times = 0;

    TestServer server(use_tcp, send_times);

    server.init();

    getchar();

    server.exit();

    return 0;
}
