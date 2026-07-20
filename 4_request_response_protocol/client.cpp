#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <netinet/ip.h>

using namespace std;

int k_max_length = 4096;

int read_all(int fd, int n, char *buf)
{
    while (n > 0)
    {
        if (fd < 0)
        {
            cout << "Error: Fd < 0" << endl;
            return -1;
        }

        ssize_t rv = read(fd, buf, n);

        if (rv <= 0)
        {
            cout << "Error or EOF: error < 0" << endl;
            return -1;
        }

        n -= rv;
        buf += rv;
    }
    return 0;
}

int write_all(int fd, char *buf, int n)
{
    while (n > 0)
    {
        if (fd < 0)
        {
            cout << "Error: Fd < 0" << endl;
            return -1;
        }

        ssize_t rv = write(fd, buf, n);

        if (rv <= 0)
        {
            cout << "Error or EOF: rv < 0 in write_all" << endl;
            return -1;
        }

        n -= rv;
        buf += rv;
    }

    return 0;
}

int query(int connfd, const char *data)
{
    if (connfd < 0)
    {
        cout << "connfd error in query" << endl;
        return -1;
    }

    uint32_t len = strlen(data);
    if (len > k_max_length)
    {
        cout << "Message is too long" << endl;
        return -1;
    }

    char wbuf[4 + k_max_length];
    memcpy(wbuf, &len, sizeof(len));
    memcpy(&wbuf[4], data, len);

    int wt_all = write_all(connfd, wbuf, len + 4);
    if (wt_all < 0)
    {
        cout << "Error in wt_all" << endl;
        return -1;
    }

    // int wt_payload = write_all(connfd, &wbuf[sizeof(len)], len);
    // if (wt_payload <= 0)
    // {
    //     cout << "Error in wt_payload" << endl;
    //     return -1;
    // }

    // check what is received
    char rbuf[4 + k_max_length];
    int rv_len = read_all(connfd, 4, rbuf);
    if (rv_len < 0)
    {
        cout << "Error in rv_len" << endl;
        return -1;
    }

    memcpy(&len, rbuf, sizeof(len));
    if (len > k_max_length)
    {
        cout << "Message is too long" << endl;
        return -1;
    }

    int rv = read_all(connfd, len, &rbuf[4]);
    if (rv < 0)
    {
        cout << "Error in rv" << endl;
        return -1;
    }


    cout << "Server Returns:" << string(&rbuf[4], len) << endl;
    return 0;
}

int main()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    if (fd < 0)
    {
        cout << "error in fd in client." << endl;
        return -1;
    }

    struct sockaddr_in addr = {};

    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    int connfd = connect(fd, (const sockaddr *)&addr, sizeof(addr));
    if (connfd < 0)
    {
        cout << "Error in connfd." << endl;
        return -1;
    }

    int q1 = query(fd, "client1");
    if (q1 < 0)
    {
        cout << "Error in q1" << endl;
        return -1;
    }

    int q2 = query(fd, "client2");
    if (q2 < 0)
    {
        cout << "Error in q2" << endl;
        return -1;
    }

    close(fd);

    return 0;
}