#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/select.h>
#include <pthread.h>
#include <assert.h>
#include <string>
#include <string.h>
#include <vector>
#include <iostream>
#include <unordered_map>
#include <fstream>
#include <sys/types.h>
#include <sstream>
#include <regex>

#include "logging.h"
#include "message.h"
#include "endpoint.h"
#include "endpoint_list.h"
#include "gomoku.h"

#define PING_INTERVAL 10

using namespace std;

static int quiting = 0;
static int g_clientfd;
static endpoint_t g_server;
static eplist_t *g_peers;
Status status = Status();

static void print_help()
{
    const static char *help_message = ""
                                      "Usage:"
                                      "\n\n login"
                                      "\n     login to server so that other peer(s) can see you"
                                      "\n\n logout"
                                      "\n     logout from server"
                                      "\n\n list"
                                      "\n     list logined peers"
                                    //   "\n\n address"
                                    //   "\n     list address book"
                                      "\n\n punch [index]"
                                      "\n     punch a hole through UDP to peer"
                                      "\n     host:port must have been logged in to server"
                                      "\n     Example:"
                                      "\n     >>> punch 0"
                                      "\n\n send data"
                                      "\n     send [data] to peer through UDP protocol"
                                      "\n     the other peer could receive your message if UDP hole punching succeed"
                                      "\n     Example:"
                                      "\n     >>> send hello"
                                      "\n\n game"
                                      "\n     invite your peer to play the Gomoku"
                                      "\n\n +x,y"
                                      "\n     set at Line x Column y"       
                                      "\n\n y"
                                      "\n     accept the invitation"
                                      "\n\n n"
                                      "\n     refuse the invitation"                           
                                      "\n\n help / ?"
                                      "\n     print this help message"
                                      "\n\n quit"
                                      "\n     logout and quit this program";
    printf("%s\n", help_message);
}

static void quit()
{
    quiting = 1;
}

void *keepalive_loop(void *temp_arr)
{
    Message ping;
    ping.head.magic = MSG_MAGIC;
    ping.head.type = MTYPE_PING;
    ping.head.length = 0;
    ping.body = NULL;
    unsigned int i = 0;
    while (!quiting)
    {
        // quit ASAP
        if (i++ < PING_INTERVAL)
        {
            sleep(1);
            continue;
        }
        i = 0;
        udp_send_msg(g_clientfd, g_server, ping);
        for (eplist_t *ep = g_peers->next; ep != NULL; ep = ep->next)
        {
            udp_send_msg(g_clientfd, ep->endpoint, ping);
        }
    }
    log_info("quiting keepalive_loop");
    return NULL;
}

void on_message(endpoint_t from, Message msg)
{
    // log_debug("RECV %d bytes FROM %s: %s %s", msg.head.length, ep_tostring(from), strmtype(msg.head.type), msg.body);
    // from server
    if (ep_equal(g_server, from))
    {
        switch (msg.head.type)
        {
        case ADDRESS:
        {
            string addr = msg.body;
            vector<string> res;
            stringstream input(addr);
            string temp;
            char pattern = ';';
            while (getline(input, temp, pattern))
            {
                if (temp.find("(you)") == temp.npos)
                    res.push_back(temp);
            }
            status.set_address(res);

            log_info("SERVER: index: address");
            status.print();
        }
        break;
        case MTYPE_PUNCH:
        {
            endpoint_t peer = ep_fromstring(msg.body);
            status.add_peer(peer);
            log_info("%s on call, replying...", ep_tostring(peer));
            udp_send_text(g_clientfd, peer, MTYPE_REPLY, NULL);
        }
        break;
        case MTYPE_REPLY:
            log_info("SERVER: %s", msg.body);
            break;
        default:
            break;
        }
        return;
    }
    // from peer
    status.add_peer(from);
    switch (msg.head.type)
    {
    case MTYPE_TEXT:
        log_info("Peer(%s): %s", ep_tostring(from), msg.body);
        break;
    case MTYPE_REPLY:
        log_info("Peer(%s) replied, you can talk now", ep_tostring(from));
        eplist_add(g_peers, from);
    case MTYPE_PUNCH:
        /*
         * Usually we can't recevie punch request from other peer directly,
         * but it could happen when it come after we reply the punch request from server,
         * or there's a tunnel already.
         * */
        udp_send_text(g_clientfd, from, MTYPE_TEXT, "I SEE YOU");
        break;
    case MTYPE_PING:
        udp_send_text(g_clientfd, from, MTYPE_PONG, NULL);
        break;

    case GAME_INVITE:
        log_info("Peer(%s) invites you to play Gomoku (y/n). You move First.", ep_tostring(from));
        status.invited();
        break;

    case GAME_ACCEPT:
    {
        log_info("Peer(%s) accepts your game invitation.", ep_tostring(from));
        // string f_ = ep_tostring(from);
        // string y_ = "You";
        status.accept(ep_tostring(from), "You", 1);
        break;
    }

    case GAME_REFUSE:
        log_info("Peer(%s) refuses your game invitation.", ep_tostring(from));
        status.refuse();
        break;

    case GAME_WIN:
        // log_info("Yor win the game.", ep_tostring(from));
        udp_send_text(g_clientfd, from, MTYPE_TEXT, "You lose the game.");
        status.refuse();
        break;

    case GAME_LOSE:
        // log_info("You Lose the game.", ep_tostring(from));
        // udp_send_text(g_clientfd, from, MTYPE_TEXT, "You win the game.");
        log_info("You win the game.");
        status.refuse();
        break;

    case GAME_TIE:
        log_info("Tie.", ep_tostring(from));
        udp_send_text(g_clientfd, from, MTYPE_TEXT, "Tie.");
        break;

    case GAME_SET:
    {
        char b[10];
        strcpy(b, msg.body);
        char *bb = b;
        char *x_ = strtok(bb, ",");
        char *y_ = strtok(NULL, " ");
        int x = atoi(x_);
        int y = atoi(y_);

        int t = status.set_(x, y);
        if (t == 2)
        {
            status.refuse();
        }

        break;
    }

    default:
        break;
    }
}

void pack_set()
{
}

void *receive_loop(void *temp_arr)
{
    endpoint_t peer;
    socklen_t addrlen;
    char buf[RECV_BUFSIZE];
    int nfds;
    fd_set readfds;
    struct timeval timeout;

    nfds = g_clientfd + 1;
    while (!quiting)
    {
        FD_ZERO(&readfds);
        FD_SET(g_clientfd, &readfds);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        int ret = select(nfds, &readfds, NULL, NULL, &timeout);
        if (ret == 0)
        {
            /* timeout */
            continue;
        }
        else if (ret == -1)
        {
            perror("select");
            continue;
        }
        assert(FD_ISSET(g_clientfd, &readfds));
        addrlen = sizeof(peer);
        memset(&peer, 0, addrlen);
        memset(buf, 0, RECV_BUFSIZE);
        int rd_size = recvfrom(g_clientfd, buf, RECV_BUFSIZE, 0,
                               (struct sockaddr *)&peer, &addrlen);
        if (rd_size == -1)
        {
            perror("recvfrom");
            continue;
        }
        else if (rd_size == 0)
        {
            log_info("EOF from %s", ep_tostring(peer));
            continue;
        }
        Message msg = msg_unpack(buf, rd_size);
        if (msg.head.magic != MSG_MAGIC || msg.body == NULL)
        {
            log_warn("Invalid message(%d bytes): {0x%x,%d,%d} %p", rd_size,
                     msg.head.magic, msg.head.type, msg.head.length, msg.body);
            continue;
        }
        on_message(peer, msg);
    }
    log_info("quiting receive_loop");
    return NULL;
}

void *console_loop(void *temp_arr)
{
    char *line = NULL;
    size_t len;
    ssize_t read;
    while (fprintf(stdout, ">>> ") && (read = getline(&line, &len, stdin)) != -1)
    {
        /* ignore empty line */

        if (read == 1)
            continue;

        regex reg("\\+\\d+,\\d+");
        string line_string = line;
        line_string = line_string.substr(0, line_string.length() - 1);
        int ret = regex_match(line_string, reg);

        char *cmd = strtok(line, " ");

        if (strncmp(cmd, "list", 4) == 0)
        {
            udp_send_text(g_clientfd, g_server, MTYPE_LIST, NULL);
        }
        else if (strncmp(cmd, "login", 5) == 0)
        {
            udp_send_text(g_clientfd, g_server, MTYPE_LOGIN, NULL);
        }
        else if (strncmp(cmd, "logout", 5) == 0)
        {
            udp_send_text(g_clientfd, g_server, MTYPE_LOGOUT, NULL);
            status.delete_peer(); // delete peer
        }
        else if (strncmp(cmd, "punch", 5) == 0)
        {
            char *index = strtok(NULL, " ");
            int i = atoi(index);
            string h_p = status.get(i);
            char *host_port = strdup(h_p.c_str());
            // cout << "host_port: " << host_port << endl;
            endpoint_t peer = ep_fromstring(host_port);
            log_info("punching %s", ep_tostring(peer));

            udp_send_text(g_clientfd, peer, MTYPE_PUNCH, NULL);
            udp_send_text(g_clientfd, g_server, MTYPE_PUNCH, host_port);
            free(host_port);
        }
        else if (strncmp(cmd, "send", 4) == 0)
        {
            char *content = strtok(NULL, "ENDL");
            udp_send_text(g_clientfd, status.get_peer(), MTYPE_TEXT, content);
        }
        else if (strncmp(cmd, "game", 4) == 0) // send game invitation to
        {
            if (!status.invite_status())
            {
                udp_send_text(g_clientfd, status.get_peer(), GAME_INVITE, NULL);
            }
        }
        else if (strncmp(cmd, "y", 1) == 0) // accept invitation
        {
            if (status.invite_status())
            {
                udp_send_text(g_clientfd, status.get_peer(), GAME_ACCEPT, NULL);

                // string f_ = ep_tostring(status.get_peer());
                // string y_ = "You";
                status.accept("You", ep_tostring(status.get_peer()), 0); // start game
            }
            else
            {
                cout << "You are not invited." << endl;
                // char invite[] = "You are not invited.";
                // char *c = invite;
                // udp_send_text(g_clientfd, status.get_peer(), MTYPE_TEXT, c);
            }
        }
        else if (strncmp(cmd, "n", 1) == 0) // refuse invitation
        {
            if (status.invite_status())
            {
                udp_send_text(g_clientfd, status.get_peer(), GAME_REFUSE, NULL);
            }
            else
            {
                cout << "You are not invited." << endl;
                // char invite[] = "You are not invited.";
                // char *c = invite;
                // udp_send_text(g_clientfd, status.get_peer(), MTYPE_TEXT, c);
                // status.refuse();
            }
        }
        else if (ret)
        {
            // cout<<"cmd "<<cmd<<endl;
            cmd = strtok(cmd, "+");

            if (status.is_gaming())
            {
                char *x_ = strtok(cmd, ",");
                char *y_ = strtok(NULL, " ");
                // cout << x_ << endl;
                // cout << y_ << endl;

                int x = atoi(x_);
                int y = atoi(y_);
                int t = status.set(x, y);
                if (t)
                {
                    char xy[10];
                    strcpy(xy, x_);
                    strcat(xy, ",");
                    strcat(xy, y_);
                    char *xy_ = xy;

                    udp_send_text(g_clientfd, status.get_peer(), GAME_SET, xy_);
                }

                if (t == 2)
                {
                    status.refuse();
                }
            }
            else
            {
                cout << "You are not in a game." << endl;
            }
        }
        else if ((strncmp(cmd, "resign", 6) == 0))
        {
            if (status.is_gaming())
            {
                udp_send_text(g_clientfd, status.get_peer(), GAME_LOSE, NULL);
                status.resign();
            }
            else
            {
                cout << "You are not in a game." << endl;
            }
        }

        else if ((strncmp(cmd, "help", 4) == 0) || (strncmp(cmd, "?", 1) == 0))
        {
            print_help();
        }
        else if (strncmp(cmd, "quit", 4) == 0)
        {
            udp_send_text(g_clientfd, g_server, MTYPE_LOGOUT, NULL);
            quit();
            break;
        }
        else
        {
            printf("Unknown command %s\n", cmd);
            // print_help();
        }
    }
    free(line);
    log_info("quiting console_loop");
    return NULL;
}

int main(int argc, char **argv)
{
    log_setlevel(INFO);
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s server:port\n", argv[0]);
        return 1;
    }
    int ret;
    pthread_t keepalive_pid, receive_pid, console_pid;

    g_server = ep_fromstring(argv[1]);
    g_peers = eplist_create();
    g_clientfd = socket(AF_INET, SOCK_DGRAM, 0);

    log_info("setting server to %s", ep_tostring(g_server));
    // print_help(); // first contact

    if (g_clientfd == -1)
    {
        perror("socket");
        goto clean;
    }
    ret = pthread_create(&keepalive_pid, NULL, &keepalive_loop, NULL);
    if (ret != 0)
    {
        perror("keepalive");
        goto clean;
    }
    ret = pthread_create(&receive_pid, NULL, &receive_loop, NULL);
    if (ret != 0)
    {
        perror("receive");
        goto clean;
    }
    ret = pthread_create(&console_pid, NULL, &console_loop, NULL);
    if (ret != 0)
    {
        perror("console");
        goto clean;
    }

    pthread_join(console_pid, NULL);
    pthread_join(receive_pid, NULL);
    pthread_join(keepalive_pid, NULL);

clean:
    close(g_clientfd);
    eplist_destroy(g_peers);
    return 0;
}
