#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include "game.h"

game* game_start(int w, int h) {  
    long size = sizeof(game);
    game* game = malloc(size);
    game->width = w;
    game->height = h;
    game->snakes_count = 0;
    game->food_count = 0;
    return game;
}

int is_snake_in(game* game, int x, int y) {
    for (int i = 0; i < game->snakes_count; i++) {
        snake* snake = game->snakes[i];
        for (int j = 0; j < snake->node_count; j++) {
            if (snake->nodes[j][0] == x && snake->nodes[j][1] == y) {
                return 1;
            }
        }
    }
    return 0;
}

snake* create_snake(game* game, struct in_addr addr) {
    for (int i = 0; i < game->snakes_count; i++) {
        if (game->snakes[i]->addr.s_addr == addr.s_addr && game->snakes[i]->alive) {
            return  game->snakes[i];// Socket already has an alive snake
        }
    }

    long size = sizeof(snake);
    snake* snake = malloc(size);
    snake->points = 1;
    snake->alive = 1;
    snake->node_count = 1;    
    snake->direction = left;
    snake->addr = addr;
    int index = game->snakes_count++;
    game->snakes[index] = snake;

    do {
        snake->nodes[0][0] = rand() % game->width;
        snake->nodes[0][1] = rand() % game->height;    
    } while (!is_snake_in(game, snake->nodes[0][0], snake->nodes[0][1]));
    return snake;
}

void spawn_foods(game* game) {
    for (int i = 0; i < MAX_FOODS; i++) {
        if (game->foods[i] == NULL) {
            food* new_food = malloc(sizeof(food));
            new_food->x = rand() % game->width;
            new_food->y =  rand() % game->height; 
            game->foods[i] = new_food;
        }
    }
    game->food_count = MAX_FOODS;
}

void write_buffer(char* buf, char c, int* pos) {
    buf[*pos] = c;
    (*pos)++;
}

int snakes_to_buf(game* game, struct in_addr addr, char* buf) {
    int pos = 0;
    write_buffer(buf, '[', &pos);
    for (int i = 0; i < game->snakes_count; i++) {
        if (i > 0) {
            write_buffer(buf, ',', &pos);
        }

        snake* snake = game->snakes[i];
        pos += sprintf(&buf[pos], "{\"is_me\":%s, \"points\":%d, \"nodes\":", addr.s_addr == snake->addr.s_addr ? "true" : "false", game->snakes[i]->points);
        write_buffer(buf, '[', &pos);
        for (int j = 0; j < snake->node_count; j++) {
            if (j > 0) {
                write_buffer(buf, ',', &pos);
            }
            pos += sprintf(&buf[pos], "[%d,%d]", snake->nodes[j][0], snake->nodes[j][1]);
        }
        write_buffer(buf, ']', &pos);
        write_buffer(buf, '}', &pos);
    }
    write_buffer(buf, ']', &pos);
    return pos;
}

int  foods_to_buf(game* game, char* buf) {
    int pos = 0;
    write_buffer(buf, '[', &pos);
    int foods_printed = 0;
    for (int i = 0; i < game->food_count; i++) {
        if (game->foods[i] != NULL) {
            if (foods_printed > 0) {
                write_buffer(buf, ',', &pos);
            }
            pos += sprintf(&buf[pos], "[%d,%d]", game->foods[i]->x, game->foods[i]->y);
            foods_printed++;
        }
    }
    write_buffer(buf, ']', &pos);
    return pos;
}

void move_snake(snake* snake, game* game) {
    int head_x = snake->nodes[0][0];
    int head_y = snake->nodes[0][1];

    int new_head_x, new_head_y;
    switch (snake->direction) {
        case up:  
            new_head_y = snake->nodes[0][1] - 1;      
            snake->nodes[0][1] = new_head_y < 0 ? game->height - 1 : new_head_y;
            break;
        case down:
            snake->nodes[0][1] = (snake->nodes[0][1] + 1) % game->height;
            break;
        case right:
            snake->nodes[0][0] = (snake->nodes[0][0] + 1) % game->width;     
            break;       
        case left:
            new_head_x = snake->nodes[0][0] - 1;
            snake->nodes[0][0] = new_head_x < 0 ? game->width - 1 : new_head_x;
            break;
    }

    for (int i = 1; i < snake->node_count; i++) {
        new_head_x = snake->nodes[i][0];
        new_head_y = snake->nodes[i][1];    
        snake->nodes[i][0] = head_x;
        snake->nodes[i][1] = head_y;
        head_x = new_head_x;  
        head_y = new_head_y;  
    }
}

void increase_size(snake* snake) {
    if (snake->node_count <= MAX_NODES) {
        snake->nodes[snake->node_count][0] = snake->nodes[0][0];
        snake->nodes[snake->node_count][1] = snake->nodes[0][1];
        snake->node_count++;
    }

    snake->points++;
}

void handle_snakes_eats(game* game) {
    for (int s = 0; s < game->snakes_count; s++) {
        for (int f = 0; f < game->food_count; f++) {            
            if (game->foods[f] != NULL 
                && game->foods[f]->x == game->snakes[s]->nodes[0][0] 
                && game->foods[f]->y == game->snakes[s]->nodes[0][1]) {
                increase_size(game->snakes[s]);
                food* food = game->foods[f];
                free(food);
                game->foods[f] = NULL;
            }
        }
    }
}

void remove_snake(snake* snake, game* game) {
    int snake_removed = 0;
    for (int i = 0; i < game->snakes_count; i++) {
        if (game->snakes[i] == snake) {
            free(snake);
            snake_removed = 1;
        }
        if (snake_removed == 1 && i < game->snakes_count - 1) {
            game->snakes[i] = game->snakes[i + 1];
        }
    }
    if (snake_removed == 1) {
        game->snakes_count--;    
    }
}

void handle_snakes_deaths(game* game) {
    snake* snake1;
    snake* snake2;
    for (int s1 = 0; s1 < game->snakes_count; s1++) {
        snake1 = game->snakes[s1];
        for (int s2 = 0; s2 < game->snakes_count; s2++) {            
            snake2 = game->snakes[s2];
            if (s1 != s2) {
                for (int i = 1; i < snake2->node_count; i++) {
                    if (snake1->nodes[0][0] == snake2->nodes[i][0] 
                        && snake1->nodes[0][1] == snake2->nodes[i][1]) {
                        snake2->points += snake1->points;
                        remove_snake(snake1, game);
                        break;
                    }
                }
            }
        }
    }
}

void move_all_snakes(game* game) {
    for (int i = 0; i < game->snakes_count; i++) {
        move_snake(game->snakes[i], game);
    }
}

int game_state_to_buf(game* game, struct in_addr addr, char* buf) {
    int pos = 0;
    pos += sprintf(&buf[pos], "{\"snakes\":");
    pos += snakes_to_buf(game, addr, &buf[pos]);
    pos += sprintf(&buf[pos], ", \"foods\":");
    pos += foods_to_buf(game, &buf[pos]);
    write_buffer(buf, '}', &pos);
    write_buffer(buf, '\0', &pos);
    return pos;
}

int game_config_to_buf(game* game, char* buf) {
    return sprintf(buf, "{\"width\":%d,\"height\":%d}", game->width, game->height);
}