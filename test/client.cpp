#include <cstdio>
#include "test_service.h"

int main(int, char *[])
{
    bool use_tcp = true;
    bool sync_connect = true;
    std::size_t send_times = 0;
    std::size_t connection_count = 1000;

    TestClient client(use_tcp, sync_connect, send_times, connection_count);

    client.init();

    getchar();

    client.exit();

    return 0;
}
