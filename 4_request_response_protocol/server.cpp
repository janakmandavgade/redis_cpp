#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
using namespace std;

int k_max_message = 4096;

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

int one_request(int connfd)
{

    if (connfd < 0)
    {
        cout << "Error on ConnFd: connfd < 0" << endl;
        return -1;
    }

    char rbuf[4 + k_max_message];

    errno = 0;

    ssize_t rv = read_all(connfd, 4, rbuf);
    if (errno)
    {
        cout << "Error or EOF: error < 0" << endl;
        return -1;
    }
    if (rv < 0)
    {
        cout << "Error or EOF: rv < 0 in one_request" << endl;
        return -1;
    }
    uint32_t len;

    memcpy(&len, rbuf, 4);
    if (len > k_max_message)
    {
        cout << "Message Too Long" << endl;
        return -1;
    }

    ssize_t rv_payload = read_all(connfd, len, &rbuf[4]);

    if (rv_payload < 0)
    {
        cout << "Error or EOF: rv_payload < 0 in one_request" << endl;
        return -1;
    }

    cout << "Client sent: " << string(&rbuf[4], len) << endl;

    // write back to wbuf and send

    char reply[] = "Server Sends Its Regards!";
    // So i will do write it in connfd

    len = strlen(reply);

    char wbuf[len + 4];

    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], reply, len);

    int wt_len = write_all(connfd, wbuf, len + 4);

    if (wt_len < 0)
    {
        cout << "wt_len <= 0 errored or EOF" << endl;
        return -1;
    }

    return 0;
}

int main()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    if (fd < 0)
    {
        cout << "Error Fd in main" << endl;
        return -1;
    }

    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    struct sockaddr_in addr = {};
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(0);
    addr.sin_family = AF_INET;

    int rv = bind(fd, (const sockaddr *)&addr, sizeof(addr));

    if (rv)
    {
        cout << "Error in bind;" << endl;
    }

    rv = listen(fd, SOMAXCONN);

    if (rv)
    {
        cout << "Error in listen;" << endl;
    }

    while (true)
    {
        struct sockaddr_in client_addr = {};
        socklen_t addrLen = sizeof(client_addr);
        int connfd = accept(fd, (struct sockaddr *)&client_addr, &addrLen);

        if (connfd < 0)
        {
            continue; // error
        }

        while (true)
        {
            int err = one_request(connfd);
            if (err)
            {
                break;
            }
        }
        close(connfd);
    }
}