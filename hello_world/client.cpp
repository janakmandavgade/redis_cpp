#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <unistd.h>

using namespace std;

int main()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in sock = {};

    sock.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    sock.sin_port = htons(10567);
    sock.sin_family = AF_INET;

    int rv = connect(fd, (const sockaddr *)&sock, sizeof(sock));

    if (rv < 0)
    {
        cout << "Unable to connect on client" << endl;
        return 1;
    }

    char wbuf[] = "Hello";
    write(fd, wbuf, sizeof(wbuf));

    char rbuf[100];

    int n = read(fd, rbuf, sizeof(rbuf) - 1);

    if (n < 0)
    {
        cout << "Error in reading :" << endl;
        return 1;
    }

    cout << "Server Says: " << rbuf << endl;
}