#include <cstdio>
#include "test_service.h"

int main(int, char *[])
{
    TestService server(false, 0, 100000);

    server.init();

    getchar();

    server.exit();

    return(0);
}
