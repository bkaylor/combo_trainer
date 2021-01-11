
#if 0

    TODOs
    [ ]

#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#include <windows.h>

#include "SDL.h"
#include "SDL_ttf.h"
#include "SDL_image.h"
#include "SDL_syswm.h"

#include "draw.h"
#include "button.h"

#define DEADZONE 0

typedef struct {
    int x;
    int y;
} Window;

typedef enum {
    LEFT,
    DOWN,
    RIGHT,
    UP,
    PUNCH,
    KICK,
    SLASH,
    HEAVY_SLASH,
    DUST,
    NONE,
} Button_Type;

typedef struct {
    int button;
    int time;

    bool should_draw;
    float distance;
} Press;

typedef struct {
    Window window;

    struct {
        int x;
        int y;
        bool clicked;
    } mouse;

    struct {
        int x;
        int y;
    } controller;

    Gui gui;
    Mouse_Info mouse_info;
    bool show_menu;

    Press recording[1000];
    int recording_index;

    unsigned int time;
    unsigned int current_time_in_recording;
    unsigned int dt;
    unsigned int starting_time;
    
    bool record;
    bool play;
    bool do_play;
    bool playing;

    bool quit;
} State;

Button_Type get_button_type(int button_id)
{
    switch (button_id)
    {
        case 13: return LEFT; break;
        case 12: return DOWN; break;
        case 14: return RIGHT; break;
        case 11: return UP; break;

        case 2: return PUNCH; break;
        case 0: return KICK; break;
        case 3: return SLASH; break;
        case 1: return HEAVY_SLASH; break;
        case 10: return DUST; break;
        default: return NONE;
    }
}

char *get_button_string(Button_Type type)
{
    switch (type)
    {
        case LEFT: return "left"; break;
        case DOWN: return "down"; break;
        case RIGHT: return "right"; break;
        case UP: return "up"; break;

        case PUNCH: return "punch"; break;
        case KICK: return "kick"; break;
        case SLASH: return "slash"; break;
        case HEAVY_SLASH: return "heavy slash"; break;
        case DUST: return "dust"; break;
        default: return "none";
    }
}

void render(SDL_Renderer *renderer, State *state)
{
    SDL_RenderClear(renderer);

    // Set background color.
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, NULL);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    float max_distance = 600;
    float top_padding = 40;
    float left_padding = 125;
    float distance_between_lanes = 50;
    float press_radius = 20;
    float heading_radius = 25;
    float draw_within_millis = 1000;

    if (state->playing)
    {
        // Decide which presses to draw.
        for (int i = 0; i < state->recording_index; i += 1)
        {
            Press *p = &state->recording[i];
            unsigned int time_until_press = p->time - state->current_time_in_recording;
            if (0 < time_until_press && time_until_press < draw_within_millis)
            {
                p->should_draw = true;
                p->distance = (float)((float)time_until_press/(float)1000) * max_distance;
            }
            else
            {
                p->should_draw = false;
            }
        }

        // Draw the incoming presses.
        for (int i = 0; i < state->recording_index; i += 1)
        {
            Press *p = &state->recording[i];
            if (p->should_draw)
            {
                draw_circle(renderer, left_padding + (get_button_type(p->button)*distance_between_lanes), p->distance + top_padding, press_radius);
            }
        }
    }

    // Draw heading.
    for (int i = 0; i < NONE; i += 1)
    {
        draw_circle(renderer, left_padding + (i*distance_between_lanes), top_padding, heading_radius);
    }

    // Draw buttons.
    draw_all_buttons(renderer, &state->gui);

    SDL_RenderPresent(renderer);
}

void update(State *state) 
{
    Gui *g = &state->gui;

    gui_frame_init(g);
    g->mouse_info = state->mouse_info;
    new_button_stack(g, 5, 5, 5);

    if (do_button_tiny(g, "?"))
    {
        state->show_menu = !state->show_menu;
    }

    if (state->show_menu)
    {
        if (!state->record)
        {
            if (do_button(g, "Record"))
            {
                printf("\n\nRECORDING\n");
                state->recording_index = 0;
                state->record = true;
            }
        }
        else
        {
            if (do_button(g, "Stop"))
            {
                printf("END RECORDING\n");
                state->record = false;
            }
        }

        if (do_button(g, "Replay"))
        {
            printf("Starting a replay ...\n");
            state->playing = true;
            state->starting_time = SDL_GetTicks();
        }

        if (do_button(g, "Print"))
        {
            printf("\n\nPLAYBACK\n");
            for (int i = 0; i < state->recording_index; i += 1)
            {
                char *button_string = get_button_string(get_button_type(state->recording[i].button));
                printf("%s at %d\n", button_string, state->recording[i].time);
            }
            printf("END PLAYBACK\n");
            state->play = false;
        }

        if (do_button(g, "Load"))
        {
            // TODO(bkaylor): Read from a file into state->recording.
        }

        if (do_button(g, "Save"))
        {
            // TODO(bkaylor): Write state->recording to a file.
        }
    }

    // Handle an in-progress Play
    if (state->playing)
    {
        state->dt = SDL_GetTicks() - state->starting_time;
        state->current_time_in_recording = (state->recording[0].time - 1000) + state->dt;

        if (state->current_time_in_recording > state->recording[state->recording_index-1].time)
        {
            printf("Play ended.\n");
            state->playing = false;
        }
    }

    return;
}

/*
void load(State *state)
{
    FILE *file = fopen("combo.txt", "r");

    char *data;
    int size;

    fseek(file, 0, seek_end);
    size = ftell(file);

    data = malloc(size+1);
    fread(data, 1, size, file);
    data[size] = 0;

    fclose(file);
}
*/

void get_input(State *state)
{
    SDL_GetMouseState(&state->mouse_info.x, &state->mouse_info.y);
    state->mouse_info.clicked = false;
    
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym)
                {
                    case SDLK_ESCAPE:
                        state->quit = true;
                        break;
                }
                break;

            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    state->mouse_info.clicked = true;
                }
                break;

            case SDL_CONTROLLERBUTTONDOWN:
                if (event.cbutton.which == 0)
                {
                    state->recording[state->recording_index] = (Press){event.cbutton.button, event.cbutton.timestamp};
                    state->recording_index += 1;

                    printf("%d button at %d\n", event.cbutton.button, event.cbutton.timestamp);
                }
                break;

            case SDL_QUIT:
                state->quit = true;
                break;

            default:
                break;
        }
    }
}

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_EVERYTHING);
    // This is meant to run in the background, need this for controller input while unfocused.
    // (Thanks Jacob :bee: on the HMN discord!)
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");

    IMG_Init(IMG_INIT_PNG);

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        printf("SDL_Init video error: %s\n", SDL_GetError());
        return 1;
    }

    if (SDL_Init(SDL_INIT_AUDIO) != 0)
    {
        printf("SDL_Init audio error: %s\n", SDL_GetError());
        return 1;
    }

    // SDL_ShowCursor(SDL_DISABLE);

	// Set up window
	SDL_Window *win = SDL_CreateWindow("Template",
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			600, 600,
			SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

	// Set up renderer
	SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	// Set up font
	TTF_Init();
	TTF_Font *font = TTF_OpenFont("liberation.ttf", 12);
	if (!font)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error: Font", TTF_GetError(), win);
		return -666;
	}

    // Set up joystick
    SDL_Joystick *joystick = NULL;

    if (SDL_NumJoysticks() < 1)
    {
        printf("No joysticks found.\n");
        return 1;
    }

    joystick = SDL_JoystickOpen(0);
    if (joystick == NULL)
    {
        printf( "Warning: Unable to open game controller! SDL Error: %s\n", SDL_GetError() );
    }

    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            printf("Joystick %i is supported by the game controller interface!\n", i);
        }
    }

    // Set up controller
    SDL_GameController *controller = NULL;
    controller = SDL_GameControllerOpen(0);
    printf("Found a valid controller, named: %s\n", SDL_GameControllerName(controller));

    // Set up main loop
    srand(time(NULL));

    // Main loop
    State state;
    state.quit = false;
    state.controller.x = 0;
    state.controller.y = 0;
    state.recording_index = 0;
    state.record = false;
    state.play = false;
    state.show_menu = false;

    // Init gui
    gui_init(&state.gui, font);

    while (!state.quit)
    {
        SDL_PumpEvents();
        get_input(&state);

        if (!state.quit)
        {
            int x, y;
            SDL_GetWindowSize(win, &state.window.x, &state.window.y);

            update(&state);
            render(ren, &state);
        }
    }

    SDL_JoystickClose(joystick);
    SDL_GameControllerClose(controller);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);
	SDL_Quit();
    return 0;
}
