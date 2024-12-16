#include <sys/socket.h>
#include <vector>
#include <poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cerrno>
#include <fcntl.h>
#include "common.h"

void do_something(int connfd) {
    char rbuf[64] = {};
    ssize_t n = read(connfd, rbuf, sizeof(rbuf) - 1);
    if (n < 0) {
        return;
    }
    printf("client says: %s\n", rbuf);

    char wbuf[] = "world";
    write(connfd, wbuf, strlen(wbuf));
}

static void fd_set_nb(int fd){
    errno = 0;
    int flags = fcntl(fd, F_GETFL, 0);
    if(errno){
        die("fcntl error");
        return;
    }

    flags |= O_NONBLOCK;

    errno = 0;
    (void)fcntl(fd, F_SETFL, flags);
    if(errno){
        die("fcntl error");
    }
}
enum {
    STATE_REQ = 0,
    STATE_RES = 1,
    STATE_END = 2,
};

struct Conn{
    int fd = -1;
    uint32_t state = 0;
    
    size_t rbuf_size = 0;
    uint8_t rbuf[4 + k_max_msg];

    size_t wbuf_size = 0;
    size_t wbuf_sent = 0;
    uint8_t wbuf[4 + k_max_msg];
};

static void conn_put(std::vector<Conn*> &f2dconn, struct Conn *conn){
    if(f2dconn.size() <= (size_t)conn->fd){
        f2dconn.resize(conn->fd + 1);
    }
    f2dconn[conn->fd] = conn;
}

static int32_t accept_new_conn(std::vector<Conn *> &f2dconn, int fd){
    //accept
    struct sockaddr_in client_addr = {};
    socklen_t socklen = sizeof(client_addr);
    int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
    if(connfd < 0){
        // printf("the error is: %s\n", strerror(errno));
        // exit(65);
        msg("accept() error.");
        return -1;
    }

    fd_set_nb(connfd);

    struct Conn* conn = (struct Conn *)malloc(sizeof(struct Conn));
    if(!conn){
        close(connfd);
        return -1;
    }
    conn->fd = connfd;
    conn->state = STATE_REQ;
    conn->rbuf_size = 0;
    conn->wbuf_size = 0;
    conn->wbuf_sent = 0;
    conn_put(f2dconn, conn);
    return 0;
}

static bool try_flush_buffer(Conn* conn){
    ssize_t rv = 0;
    do{
        size_t remain = conn->wbuf_size - conn->wbuf_sent;
        rv = write(conn->fd, &conn->wbuf[conn->wbuf_sent], remain);
    } while (rv < 0 && errno == EINTR);

    if(rv < 0 && errno == EAGAIN){
        return false;
    }
    if(rv < 0){
        msg("write() error");
        conn->state = STATE_END;
        return false;
    }
    conn->wbuf_sent += (size_t)rv;
    assert(conn->wbuf_sent <= conn->wbuf_size);
    if(conn->wbuf_sent == conn->wbuf_size){
        conn->state = STATE_REQ;
        conn->wbuf_sent = 0;
        conn->wbuf_size = 0;
        return false;
    }
    //still some data in wbuf and need to retry
    return true;
}

static void state_res(Conn* conn){
    while(try_flush_buffer(conn)){}
}

static bool try_one_request(Conn* conn){
    if(conn->rbuf_size < 4){
        return false;
    }

    uint32_t len = 0;
    memcpy(&len, &conn->rbuf[0], 4);

    if(len > k_max_msg){
        msg("This message is too long");
        conn->state = STATE_END;
        return false;
    }

    if(4 + len > conn->rbuf_size){
        return false;
    }

    //enough data to process finally
    printf("The client says: %s\n", &conn->rbuf[4]);

    //generate a response
    memcpy(&conn->wbuf[0], &len, 4);
    memcpy(&conn->wbuf[4], &conn->rbuf[4], len);
    conn->wbuf_size = 4 + len;

    size_t remain = conn->rbuf_size - 4 - len;
    if(remain){
        memmove(conn->rbuf, &conn->rbuf[4 + len], remain);
    }
    conn->rbuf_size = remain;

    //change state
    conn->state = STATE_RES;
    state_res(conn);

    return (conn->state == STATE_REQ);
}

static bool try_fill_buffer(Conn* conn){
    assert(conn->rbuf_size < sizeof(conn->rbuf));
    ssize_t rv = 0;
    do {
        size_t cap = sizeof(conn->rbuf) - conn->rbuf_size;
        rv = read(conn->fd, &conn->rbuf[conn->rbuf_size], cap);
    } while (rv < 0 && errno == EINTR);

    if(rv < 0 && errno == EAGAIN){
        return false;
    }
    if(rv < 0){
        msg("read() error.");
        return false;
    }
    if(rv == 0){
        if(conn->rbuf_size > 0){
            msg("unexpected EOF");
        }
        else{
            msg("EOF");
        }
        conn->state = STATE_END;
        return false;
    }
    
    conn->rbuf_size += (size_t)rv;
    assert(conn->rbuf_size <= sizeof(conn->rbuf));

    //try to process requests
    while (try_one_request(conn)) {}
    return (conn->state == STATE_REQ);
}

static void state_req(Conn* conn){
    while (try_fill_buffer(conn)) {}
}

static void connection_io(Conn* conn){
    if(conn->state == STATE_REQ){
        state_req(conn);
    }
    else if(conn->state == STATE_RES){
        state_res(conn);
    }
    else{
        assert(0);
    }
}

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    // Bind
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(0);
    int rv = bind(fd, (const sockaddr *)&addr, sizeof(addr));

    rv = listen(fd, SOMAXCONN);
    if (rv) {
        return -1;
    }

    std::vector<Conn *>fd2Conn;
    fd_set_nb(fd);

    std::vector<struct pollfd> poll_args;
    while (true) {
        //prepare arguments of the poll()
        poll_args.clear();
        //listening fd put in the first position
        struct pollfd pfd = {fd, POLL_IN, 0};
        poll_args.push_back(pfd);
        //connection fds
        for(Conn* conn : fd2Conn){
            if(!conn){
                continue;
            }
            struct pollfd pfd = {};
            pfd.fd = conn->fd;
            pfd.events = (conn->state == STATE_REQ) ? POLLIN : POLLOUT;
            pfd.events = pfd.events | POLLERR;
            poll_args.push_back(pfd);
        }

        //poll for active fds
        int rv = poll(poll_args.data(), (nfds_t)poll_args.size(), 1000);
        if(rv < 0){
            die("poll");
        }

        //process active connections
        for(size_t i = 1; i < poll_args.size(); i++){
            if(poll_args[i].revents){
                Conn* conn = fd2Conn[poll_args[i].fd];
                connection_io(conn);
                if(conn->state == STATE_END){
                    fd2Conn[conn->fd] = NULL;
                    (void)close(fd);
                    free(conn);
                }
            }
        }

        //try to accept a new connection if listening fd active
        if(poll_args[0].revents){
            (void)accept_new_conn(fd2Conn, fd);
        }
    }
    return 0;
}