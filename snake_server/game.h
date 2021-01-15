#ifndef GAME_GUARD
#define GAME_GUARD

#define MAX_SNAKES 10
#define MAX_NODES 15
#define MAX_FOODS 5
#define DEFAULT_WIDTH 30
#define DEFAULT_HEIGHT 30
#define FOOD_SPAWN_RATE 100

enum move {
    up,
    down,
    right,
    left
};

typedef struct snake {
    struct in_addr addr;
    int points;
    int node_count;
    int alive;
    enum move direction;
    int nodes[MAX_NODES][2];
} snake;

typedef struct food {
    int x;
    int y;
} food;

typedef struct game {
    int width;
    int height;
    int snakes_count;
    int food_count;
    snake* snakes[MAX_SNAKES];
    food* foods[MAX_FOODS];
} game;

// Game functions
game* game_start(int w, int h);
int is_snake_in(game* game, int x, int y);
void handle_snakes_eats(game* game);
void handle_snakes_deaths(game* game);
void spawn_foods(game* game);
void move_all_snakes(game* game);

// Snake functions
snake* create_snake(game* game, struct in_addr addr);
void move_snake(snake* snake, game* game);
void increase_size(snake* snake);
void remove_snake(snake* snake, game* game);

// Utility functions
void write_buffer(char* buf, char c, int* pos);

// Output functions
int snakes_to_buf(game* game, struct in_addr addr, char* buf);
int foods_to_buf(game* game, char* buf);
int game_state_to_buf(game* game, struct in_addr addr, char* buf);
int game_config_to_buf(game* game, char* buf);

#endif