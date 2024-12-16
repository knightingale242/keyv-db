#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cerrno>
#include "common.h"

static int32_t send_req(int fd, const char* text) {
    uint32_t len = (uint32_t)strlen(text);
    if (len > k_max_msg) {
        return -1; // Message too long
    }

    // Prepare the write buffer
    char wbuf[4 + len];
    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], text, len);

    // Send request
    int32_t err = write_full(fd, wbuf, 4 + len);
    return err; // Return 0 on success, or an error code
}


static int32_t read_res(int fd) {
    uint32_t len;
    char rbuf[4 + k_max_msg + 1];

    // Read the length of the response
    errno = 0;
    int32_t err = read_full(fd, rbuf, 4);
    if (err) {
        if (errno == 0) {
            msg("EOF");
        } else {
            msg("read() error");
        }
        return err;
    }

    memcpy(&len, rbuf, 4);
    if (len > k_max_msg) {
        msg("This message is too long");
        return -1; // Message length invalid
    }

    // Read the actual message
    err = read_full(fd, &rbuf[4], len);
    if (err) {
        msg("read() error");
        return err;
    }
    rbuf[4 + len] = '\0';
    printf("server says: %s\n", &rbuf[4]);

    return 0; // Success
}

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return -1;
    }

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);
    int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv < 0) {
        return -1;
    }

    //multiple pipelined reqs
    const char* query_list[3] = {"hello1", "hello2", "hello3"};
    for(size_t i = 0; i < 3; i++){
        int32_t err = send_req(fd, query_list[i]);
        if(err){
            goto L_DONE;
        }
    }
    for (size_t i = 0; i < 3; i++){
        int32_t err = read_res(fd);
        if(err){
            goto L_DONE;
        }
    }
L_DONE:
    close(fd);
    return 0;
}