#include <cstdio>
#include "test_service.h"

int main(int, char *[])
{
    TestService client(true, 0, 100000);

    client.init();

    getchar();

    client.exit();

    return(0);
}
