#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h> // For close()
#include <cstdlib>  // For exit()
#include <cstring>
using namespace std;

static void doSomething(int connfd)
{
    char rbuf[100] = {};
    // memset(rbuf, 0, sizeof(rbuf));

    int rd = read(connfd, rbuf, sizeof(rbuf) - 1);

    if (rd <= 0)
    {
        cout << "rd < 0 " << endl;
        return;
    }

    cout << "Client Data:" << rbuf << endl;
    rbuf[rd] = '\0';

    char wbuf[] = "World";
    write(connfd, wbuf, sizeof(wbuf));
}

int main()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    if (fd < 0)
    {
        cout << "fd < 0 && socket()" << endl;
        return 1;
    }

    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    sockaddr_in serverAddress = {};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(10567);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    int rv = bind(fd, (struct sockaddr *)&serverAddress, sizeof(serverAddress));

    if (rv < 0)
    {
        cout << "rv < 0" << endl;
        return 1;
    }

    listen(fd, SOMAXCONN);

    while (true)
    {
        sockaddr_in client_addr = {};
        socklen_t addrlen = sizeof(client_addr);

        int connfd = accept(fd, (struct sockaddr *)&client_addr, &addrlen);

        if (connfd < 0)
        {
            cout << "accept < 0" << endl;
            continue;
        }

        doSomething(connfd);

        close(connfd);
    }
}