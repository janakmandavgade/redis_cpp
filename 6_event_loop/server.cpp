// stdlib
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
// system
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
// C++
#include <iostream>
#include <vector>
using namespace std;

const size_t k_max_msg = 32 << 20;

static void fd_set_nb(int fd)
{
    errno = 0;
    int flag = fcntl(fd, F_GETFL, 0);
    if (errno)
    {
        cout << "Error in fd_set_nb in ::28:: " << endl;
        return;
    }

    flag |= O_NONBLOCK;

    flag = fcntl(fd, F_SETFL, flag);

    if (errno)
    {
        cout << "Error in fd_set_nb in ::37::" << endl;
        return;
    }
}

struct Conn
{
    int fd = -1;

    bool want_read = false;
    bool want_write = false;
    bool want_close = false;

    vector<uint8_t> incoming;
    vector<uint8_t> outgoing;
};

static void buf_append(vector<uint8_t> &buf, const uint8_t *data, size_t len)
{
    buf.insert(buf.end(), data, data + len);
}

static void buf_consume(vector<uint8_t> &buf, size_t n)
{
    buf.erase(buf.begin(), buf.begin() + n);
}

static Conn *handle_accept(int fd)
{
    struct sockaddr_in client_addr = {};

    // addr.sin_family = AF_INET;
    // addr.sin_port = ntohs(1234);
    // addr.sin_addr.s_addr = ntohl(0);

    socklen_t addrlen = sizeof(client_addr);
    int connfd = accept(fd, (struct sockaddr *)&client_addr, &addrlen);

    if (connfd < 0)
    {
        cout << "Connfd < 0 in handle_accept" << endl;
        return {};
    }

    uint32_t ip = client_addr.sin_addr.s_addr;
    fprintf(stderr, "new client from %u.%u.%u.%u:%u\n",
            ip & 255, (ip >> 8) & 255, (ip >> 16) & 255, ip >> 24,
            ntohs(client_addr.sin_port));

    fd_set_nb(connfd);

    Conn *conn = new Conn();
    conn->fd = fd;
    conn->want_read = true;

    return conn;
}

static bool try_one_request(Conn *conn)
{
    int fd = conn->fd;

    if (conn->incoming.size() < 4)
    {
        return false; // want read
    }

    uint32_t len = 0;
    memcpy(&len, conn->incoming.data(), 4);
    if (len > k_max_msg)
    {
        cout << "too long" << endl;
        conn->want_close = true;
        return false; // want close
    }

    // message body
    if (4 + len > conn->incoming.size())
    {
        return false; // want read
    }
    const uint8_t *request = &conn->incoming[4];

    printf("client says: len:%d data:%.*s\n",
           len, len < 100 ? len : 100, request);

    buf_append(conn->outgoing, (const uint8_t *)&len, 4);
    buf_append(conn->outgoing, request, len);

    buf_consume(conn->incoming, len + 4);

    return true;
}

static void handle_write(Conn *conn)
{
    assert(conn->outgoing.size() > 0);

    int rv = write(conn->fd, &(conn->outgoing[0]), conn->outgoing.size());

    if (rv < 0 || errno == EAGAIN)
    {
        return;
    }

    if (rv < 0)
    {
        cout << "Error in rv in handle_write" << endl;
        conn->want_close = true;
        return;
    }

    buf_consume(conn->outgoing, size_t(rv));

    // update the readiness intention
    if (conn->outgoing.size() == 0)
    { // all data written
        conn->want_read = true;
        conn->want_write = false;
    }

    return;
}

static void handle_read(Conn *conn)
{
    assert(conn->incoming.size() > 0);

    uint8_t buf[64 * 1024];
    int len;
    int rv = read(conn->fd, buf, sizeof(buf));

    if (rv < 0 || errno == EAGAIN)
    {
        return;
    }

    if (rv < 0)
    {
        cout << "Error in 1st rv in handle_read" << endl;
        conn->want_close = true;
        return;
    }

    if (rv == 0)
    {
        if (conn->incoming.size() == 0)
        {
            cout << "Client closed" << endl;
        }
        else
        {
            cout << "Unexpected EOF" << endl;
        }
        conn->want_close = true;
        return;
    }

    buf_append(conn->incoming, buf, size_t(rv));
    while (try_one_request(conn))
    {
    }

    if (conn->outgoing.size() > 0)
    {
        conn->want_read = true;
        conn->want_write = false;

        return handle_write(conn);
    }

    return;
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