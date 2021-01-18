#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "game.c"

// Data passed to client thread
typedef struct client_data {
    int sockfd;
    game* g;
    pthread_mutex_t* mutex;
    pthread_t* t;
    struct in_addr addr;
} client_data;

// Data passed to game thread
typedef struct game_data {
    game* g;
    pthread_mutex_t* mutex;
} game_data;

void* game_thread(game_data* g_data);
void* client_thread(client_data* c_data);
void handle_client_request(struct in_addr addr, int sockfd, char* buf, snake* s, game* g);

int main(int argc, char *argv[]) {
    game* g = game_start(DEFAULT_WIDTH, DEFAULT_WIDTH);

    int sockfd, new_sockfd;
    pid_t child_pid;
    char buf[BUFSIZ];
    struct sockaddr_in serv, cInt;
    socklen_t sin_siz;
    int port;
    pthread_t client_threads[MAX_SNAKES];
    pthread_t game_t;
    pthread_mutex_t mutex;

    game_data* g_data = malloc(sizeof(game_data));
    g_data->g = g;
    g_data->mutex = &mutex;

    pthread_mutex_init(&mutex, NULL);
    // Creating game thread
    pthread_create(&game_t, NULL, (void *) game_thread, (void *) g_data);

    if (argc != 3) {
        printf("Usage: ./server <host> <port>\n");
        exit(1);
    }

    if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    serv.sin_family = PF_INET;
    port = strtol(argv[2], NULL, 10);
    serv.sin_port = htons(port);
    inet_aton(argv[1], &serv.sin_addr);
    sin_siz = sizeof(struct sockaddr_in);

    if (bind(sockfd, (struct sockaddr*) &serv, sizeof(serv)) < 0) {
        perror("bind");
        exit(1);
    }
    printf("bind() called\n");

    if (listen(sockfd, SOMAXCONN) < 0) {
        perror("listen");
        exit(1);
    }
    printf("listen() called\n");

    int i;

    while (1) {
        if ((new_sockfd = accept(sockfd, (struct sockaddr*) &cInt, &sin_siz)) < 0) {
            perror("accept");
        }

        client_data* c_data = malloc(sizeof(client_data));
        c_data->sockfd = new_sockfd;
        c_data->g = g;
        c_data->mutex = &mutex;
        c_data->addr = cInt.sin_addr;

        // Verify if there's thread slots for new client
        for (i = 0; i < MAX_SNAKES; i++) {
            if (client_threads[i] == 0) {
                // Create new thread for the client
                c_data->t = &client_threads[i];
                pthread_create(&client_threads[i], NULL, (void *) client_thread, (void *) c_data);
                printf("Client allocated to thread %d.\n", i);
                break;
            }
        }

        // If all thread slots are used
        if (i == MAX_SNAKES - 1) {
            close(new_sockfd);
            free(c_data);
        }
    }
    close(sockfd);
    return 0;
}

void* game_thread(game_data* g_data) {    
    int ticks = 0;
    char buf[1000];

    struct in_addr* addr = malloc(sizeof(struct in_addr));
    addr->s_addr = 1;

    while (1) {
        pthread_mutex_lock(g_data->mutex);
        if (ticks % FOOD_SPAWN_RATE == 0) {
            spawn_foods(g_data->g);
        }
        move_all_snakes(g_data->g);
        handle_snakes_deaths(g_data->g);
        handle_snakes_eats(g_data->g);

        game_state_to_buf(g_data->g, *addr, buf);
        printf("%s\n", buf);


        ticks++;
        pthread_mutex_unlock(g_data->mutex);        
        sleep(1);
    }
    free(g_data->g);
}

void* client_thread(client_data* c_data) {
    pthread_mutex_lock(c_data->mutex);
    snake* client_snake = create_snake(c_data->g, c_data->addr);
    pthread_mutex_unlock(c_data->mutex);

    int len = 0;
    char buf[BUFSIZ];

    while (1) {
        memset(buf, 0, BUFSIZ);
        len = recv(c_data->sockfd, buf, BUFSIZ, 0);
        if (len == 0) {
            break;
        }
        buf[len] = '\0';

        pthread_mutex_lock(c_data->mutex);        
        handle_client_request(c_data->addr, c_data->sockfd, buf, client_snake, c_data->g);
        pthread_mutex_unlock(c_data->mutex);
    }
    pthread_mutex_lock(c_data->mutex);
    close(c_data->sockfd);
    *c_data->t = 0;
    free(c_data);
    pthread_mutex_unlock(c_data->mutex);
}

void handle_client_request(struct in_addr addr, int sockfd, char* buf, snake* s, game* g) {
    if (strncasecmp("GET ", buf, 4) == 0) {
        char relative_parts[2][20];
        int i = 5;
        int j = 0;
        int str_b = 0;
        while (1) {
            if (buf[i] == '/') {
                relative_parts[j][str_b] = '\0';
                j++;
                str_b = 0;
            } 
            else if (buf[i] == ' ') {
                relative_parts[j][str_b] = '\0';
                break;
            }
            else {
                relative_parts[j][str_b] = buf[i];
                str_b++;
            }            
            i++;
        }

        printf("/%s/%s", relative_parts[0], relative_parts[1]);

        // GET /game_state
        if (strncasecmp("game_state", relative_parts[0], 10) == 0) {
            memset(buf, 0, BUFSIZ);
            
            int buf_size = sprintf(buf, "HTTP/1.1 200 OK\r\nContent-Length:        \r\nAccess-Control-Allow-Origin: *\r\nContent-Type:application/json\r\n\r\n");
            int pos = game_state_to_buf(g, addr, &buf[buf_size]);
            int new_pos = sprintf(&buf[32], "%d", pos - 1);
            buf[32 + new_pos] = ' ';
            send(sockfd, buf, buf_size + pos - 1, 0);
        }

        // GET /move/<MOVE>
        else if (strncasecmp("move", relative_parts[0], 4) == 0) {
            switch (relative_parts[1][0]) {
                case '0':
                    s->direction = up;
                    break;
                case '1':
                    s->direction = down;
                    break;
                case '2':
                    s->direction = right;
                    break;
                case '3':
                    s->direction = left;
                    break;
            }
            send(sockfd, "HTTP/1.1 200 OK\r\nContent-Length: 2\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\n\r\nOK", 105, 0);
        } 

        // GET /game_config
        else if (strncasecmp("game_config", relative_parts[0], 11) == 0) {
            int buf_size = sprintf(buf, "HTTP/1.1 200 OK\r\nContent-Length:        \r\nAccess-Control-Allow-Origin: *\r\nContent-Type:application/json\r\n\r\n");
            int pos = game_config_to_buf(g, &buf[buf_size]);
            int new_pos = sprintf(&buf[32], "%d", pos);
            buf[32 + new_pos] = ' ';
            printf("%s\n", buf);
            send(sockfd, buf, buf_size + pos, 0);
        }
    }
}