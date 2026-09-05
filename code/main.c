#include <SDL3/SDL.h>


#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

#define VIRTUAL_WIDTH 200 
#define VIRTUAL_HEIGHT 120 

#define SNAKE_CELL_SIZE 5

#define GRID_WIDTH 40
#define GRID_HEIGHT 24

#define MAX_COMMANDS 100


typedef enum
{
    CMD_CLEAR,
    CMD_RECT,
}CMD_TYPE;

typedef struct
{
    int r, g, b, a;
}draw_clear;

typedef struct
{
    int x, y, w, h;
    int r, g, b, a;
}draw_rect;

typedef struct
{
    CMD_TYPE type;
    union
    {
	draw_rect rect;
	draw_clear clear;
    };
}cmd;

typedef struct
{
    uint8_t r, g, b, a;
}color;

typedef struct
{
    uint32_t length;
    cmd commands[MAX_COMMANDS];
}cmd_buffer;

typedef struct
{
    int x;
    int y;
}coords;

typedef struct
{
    int length;
    coords body[100];
}snake;

typedef struct
{
    uint8_t up, down, left, right;    
}input;

typedef struct
{
    snake s;
    coords food;
    cmd_buffer cbuf;
    float delta;
    coords dir;
    input in;
    uint8_t should_quit;
}game_state;

void QuitSDL(SDL_Renderer *Renderer, SDL_Window *Window)
{
    SDL_DestroyRenderer(Renderer);
    SDL_DestroyWindow(Window);
    SDL_Quit();
}

SDL_FRect Tilemap[15][24];
SDL_Texture *Tileset;
SDL_Renderer *Renderer;
SDL_Window *Window;

static void DrawClear(cmd_buffer *commandBuffer, int r, int g, int b, int a)
{
    cmd command = 
    {
	.type = CMD_CLEAR,
	.clear = {r, g, b, a},
    };
    commandBuffer->commands[commandBuffer->length++] = command;
}

static void DrawRect(cmd_buffer *commandBuffer,  int x, int y, int w, int h, int r, int g, int b, int a)
{
    cmd command = 
    {
	.type = CMD_RECT,
	.rect = {x, y, w, h, r, g, b, a},
    };
    commandBuffer->commands[commandBuffer->length++] = command;
}

static void HandleInput(game_state *state)
{
    if(state->in.up && state->dir.y != 1)
    {
	state->dir = (coords){0, -1};
    }
    else if(state->in.down && state->dir.y != -1)
    {
	state->dir = (coords){0, 1};
    }
    else if(state->in.right && state->dir.x != -1)
    {
	state->dir = (coords){1, 0};
    }
    else if(state->in.left && state->dir.x != 1)
    {
	state->dir = (coords){-1, 0};
    }
}

static void MoveSnake(snake *s, coords dir)
{
    for(int snake_cell = s->length - 1; snake_cell >= 1; --snake_cell)
    {
	s->body[snake_cell] = s->body[snake_cell - 1];	
    }

    s->body[0].x += dir.x;
    s->body[0].y += dir.y;
}

static void DrawSnake(cmd_buffer *commandBuffer, snake *s)
{
    DrawRect(commandBuffer, 5 * s->body[0].x, 5 * s->body[0].y, SNAKE_CELL_SIZE, SNAKE_CELL_SIZE, 45, 80, 42, 255);
    for(int snake_cell = 1; snake_cell < s->length; ++snake_cell)
    {
	DrawRect(commandBuffer, 5 * s->body[snake_cell].x, 5 * s->body[snake_cell].y, SNAKE_CELL_SIZE, SNAKE_CELL_SIZE, 45, 100, 42, 255);
    }
}

static void CheckCollision(game_state *state)
{
    snake *s = &state->s;
    coords snakeHead = s->body[0];

    if(snakeHead.x > GRID_WIDTH || snakeHead.x < 0 || snakeHead.y > GRID_HEIGHT || snakeHead.y < 0)
    {
	state->should_quit = 1;
	return;
    }
    for(int snakeCell = 1; snakeCell < s->length; ++snakeCell)
    {
	if(snakeHead.x == s->body[snakeCell].x && snakeHead.y == s->body[snakeCell].y)
	{
	    state->should_quit = 1;
	    return;
	}
    }
}

#define NUM_OF_PARTICLES 15

typedef struct
{
    float x, y, vx, vy;
}particle;

particle particles[NUM_OF_PARTICLES];
int time = 2;
coords velocity = {5, 0};
int showParticles = 0;
int particle_timer = 0; 

#define PARTICLE_DURATION 1000

#define VELOCITY 50

static void MakeExplode(game_state *state, int x, int y)
{
    for(int i = 0; i < NUM_OF_PARTICLES; ++i)
    {
	particles[i] = (particle){5 * x + 2,  5 * y + 2, SDL_rand(VELOCITY) - VELOCITY / 2, SDL_rand(VELOCITY) - VELOCITY / 2};
    }
    particle_timer = PARTICLE_DURATION;
    showParticles = 1;
}

static void DrawParticles(game_state *state)
{
    particle_timer -= state->delta; 

    if(particle_timer <= 0)
    {
	showParticles = 0;
    }

    for(int i = 0; i < NUM_OF_PARTICLES; ++i)
    {
	particles[i].x += particles[i].vx * (state->delta / 1000.0f);
	particles[i].y += particles[i].vy * (state->delta / 1000.0f);

	DrawRect(&state->cbuf, particles[i].x, particles[i].y, 1, 1, 255, 0, 0, 255.0f * ((float)particle_timer / PARTICLE_DURATION));
    }
}

static void GameUpdateAndRender(game_state *state)
{
    static float timer = 0.0f;
    timer += state->delta;
    snake *s = &state->s;
    coords food = state->food;
    coords snakeHead = s->body[0];
    
    DrawClear(&state->cbuf, 45, 130, 42, 255);
    DrawRect(&state->cbuf, 5 * state->food.x, 5 * state->food.y, SNAKE_CELL_SIZE, SNAKE_CELL_SIZE, 255, 0, 0, 255);    

    CheckCollision(state);


    if(timer >= 100.0f)
    {
	HandleInput(state);
	coords next_cell = (coords){snakeHead.x + state->dir.x, snakeHead.y + state->dir.y};

	uint8_t addFood = 0;
	if(next_cell.x == food.x && next_cell.y == food.y)
	{
	    coords snakeTail = s->body[s->length - 1];
	    s->body[s->length] = snakeTail;
	    addFood = 1;
	    MakeExplode(state, food.x, food.y);
	    state->food.x = (snakeHead.x * 30 % GRID_WIDTH + 17) % GRID_WIDTH;
	    state->food.y = (snakeHead.y * 30 % GRID_HEIGHT + 19) % GRID_HEIGHT;
	}

	MoveSnake(s, state->dir);

	if(addFood)
	{
	    ++s->length;
	}

	timer = 0.0f;
    }
    DrawSnake(&state->cbuf, s);

    if(showParticles)
    {
	DrawParticles(state);
    }
}

static game_state InitGameState()
{
    game_state state = {};
    state.s.length = 0; 
    state.food.x = 5;
    state.food.y = 10;
    state.cbuf.length = 0;

    state.s.body[0] = (coords){12, 11};
    state.s.body[1] = (coords){11, 11};
    state.s.body[2] = (coords){10, 11};
    state.s.length = 3;

    state.dir = (coords){1, 0};
    state.should_quit = 0;
    return state;
}


int main(int argc, char *argv[])
{
    if(!SDL_Init(SDL_INIT_VIDEO))
    {
	return 0;
    }

    Window = SDL_CreateWindow("Resolution", WINDOW_WIDTH, WINDOW_HEIGHT, 0);

    if(Window == 0)
    {
	SDL_Quit();
    }

    Renderer = SDL_CreateRenderer(Window, 0);

    if(Renderer == 0)
    {
	SDL_DestroyWindow(Window);
	SDL_Quit();
    }
    
    SDL_SetRenderLogicalPresentation(Renderer, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, SDL_LOGICAL_PRESENTATION_STRETCH);

    int Running = 1;
    SDL_Event Event;

    input in = {};
    game_state state = InitGameState();

    unsigned int last = 0;
    unsigned int current = 0;

    SDL_srand(0);

    while(!state.should_quit)
    {
	while(SDL_PollEvent(&Event))
	{
	    if(Event.type == SDL_EVENT_QUIT)
	    {
		state.should_quit = 1;
	    }
	    else if(Event.type == SDL_EVENT_KEY_DOWN)
	    {
		if(Event.key.key == SDLK_ESCAPE)
		{
		    state.should_quit = 1;
		}

		if(Event.key.key == SDLK_UP)
		{
		    in.up = 1;
		    in.down = 0;
		    in.right = 0;
		    in.left = 0;
		}
		else if(Event.key.key == SDLK_DOWN)
		{
		    in.up = 0;
		    in.down = 1;
		    in.right = 0;
		    in.left = 0;
		}
		else if(Event.key.key == SDLK_RIGHT)
		{
		    in.up = 0;
		    in.down = 0;
		    in.right = 1;
		    in.left = 0;

		}
		else if(Event.key.key == SDLK_LEFT)
		{
		    in.up = 0;
		    in.down = 0;
		    in.right = 0;
		    in.left = 1;
		}
	    }
	}

	current = SDL_GetTicks();

	float delta = (current - last);

	state.in = in;
	state.delta = delta;
	GameUpdateAndRender(&state);	


	SDL_SetRenderDrawBlendMode(Renderer, SDL_BLENDMODE_BLEND);
	for(int commandIndex = 0; commandIndex < state.cbuf.length; ++commandIndex)
	{
	    cmd command = state.cbuf.commands[commandIndex];
	    switch(command.type)
	    {
	    case CMD_CLEAR:
		SDL_SetRenderDrawColor(Renderer, command.clear.r, command.clear.g, command.clear.b, command.clear.a);
		SDL_RenderClear(Renderer);
		break;
	    case CMD_RECT:
		SDL_SetRenderDrawColor(Renderer, command.rect.r, command.rect.g, command.rect.b, command.rect.a);
		SDL_FRect rect = 
		{
		    .x = command.rect.x,
		    .y = command.rect.y,
		    .w = command.rect.w,
		    .h = command.rect.h,
		};
		SDL_RenderFillRect(Renderer, &rect);
		break;
	    }
	}
	state.cbuf.length = 0;

	SDL_RenderPresent(Renderer);

	last = current;
    }

    SDL_DestroyRenderer(Renderer);


    SDL_DestroyWindow(Window);
    SDL_Quit();

    return 0;
}
