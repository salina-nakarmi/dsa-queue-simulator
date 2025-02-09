#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include "../include/queue.h"

// Window and graphics constants
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800
#define ROAD_WIDTH 150
#define LANE_WIDTH 50
#define VEHICLE_WIDTH 30
#define VEHICLE_HEIGHT 20
#define PORT 8000
#define FONT_SIZE 24
#define FONT_PATH "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"

// Global variables
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
TTF_Font* font = NULL;
VehicleQueue laneQueues[5];  // AL1, BL1, CL1, DL1, AL2
const char* LANE_NAMES[] = {"AL1", "BL1", "CL1", "DL1", "AL2"};

// Function declarations
bool initSDL(void);
void drawText(const char* text, int x, int y);
void drawRoads(void);
void drawVehicle(int x, int y, SDL_Color color, const char* lane);
void drawTrafficLight(int x, int y, int state);
void updateTrafficLights(void);
void processVehicles(void);
void* networkThread(void* arg);
void updateAndRender(void);
void cleanup(void);

bool initSDL(void) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        return false;
    }

    if (TTF_Init() < 0) {
        fprintf(stderr, "TTF initialization failed: %s\n", TTF_GetError());
        SDL_Quit();
        return false;
    }

    window = SDL_CreateWindow("Traffic Junction Simulator",
                           SDL_WINDOWPOS_CENTERED,
                           SDL_WINDOWPOS_CENTERED,
                           WINDOW_WIDTH, WINDOW_HEIGHT,
                           SDL_WINDOW_SHOWN);
    if (!window) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return false;
    }

    font = TTF_OpenFont(FONT_PATH, FONT_SIZE);
    if (!font) {
        fprintf(stderr, "Font loading failed: %s\n", TTF_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return false;
    }

    return true;
}

void drawText(const char* text, int x, int y) {
    SDL_Color textColor = {0, 0, 0, 255};
    SDL_Surface* surface = TTF_RenderText_Solid(font, text, textColor);
    if (surface) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (texture) {
            SDL_Rect dest = {x, y, surface->w, surface->h};
            SDL_RenderCopy(renderer, texture, NULL, &dest);
            SDL_DestroyTexture(texture);
        }
        SDL_FreeSurface(surface);
    }
}

void drawRoads(void) {
    // Draw background
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    // Draw roads
    SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
    SDL_Rect verticalRoad = {WINDOW_WIDTH/2 - ROAD_WIDTH/2, 0, ROAD_WIDTH, WINDOW_HEIGHT};
    SDL_Rect horizontalRoad = {0, WINDOW_HEIGHT/2 - ROAD_WIDTH/2, WINDOW_WIDTH, ROAD_WIDTH};
    SDL_RenderFillRect(renderer, &verticalRoad);
    SDL_RenderFillRect(renderer, &horizontalRoad);

    // Draw lane markings
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int i = 1; i < 3; i++) {
        // Vertical lane markings
        SDL_RenderDrawLine(renderer,
            WINDOW_WIDTH/2 - ROAD_WIDTH/2 + i*LANE_WIDTH, 0,
            WINDOW_WIDTH/2 - ROAD_WIDTH/2 + i*LANE_WIDTH, WINDOW_HEIGHT);

        // Horizontal lane markings
        SDL_RenderDrawLine(renderer,
            0, WINDOW_HEIGHT/2 - ROAD_WIDTH/2 + i*LANE_WIDTH,
            WINDOW_WIDTH, WINDOW_HEIGHT/2 - ROAD_WIDTH/2 + i*LANE_WIDTH);
    }

    // Draw lane labels
    drawText("A", WINDOW_WIDTH/2 - 10, 20);
    drawText("B", WINDOW_WIDTH - 40, WINDOW_HEIGHT/2 - 10);
    drawText("C", WINDOW_WIDTH/2 - 10, WINDOW_HEIGHT - 40);
    drawText("D", 20, WINDOW_HEIGHT/2 - 10);
}

void drawVehicle(int x, int y, SDL_Color color, const char* lane) {
    SDL_Rect vehicle;
    // Adjust vehicle orientation based on lane
    if (strncmp(lane, "A", 1) == 0 || strncmp(lane, "C", 1) == 0) {
        vehicle = (SDL_Rect){x - VEHICLE_WIDTH/2, y - VEHICLE_HEIGHT,
                            VEHICLE_WIDTH, VEHICLE_HEIGHT * 1.5}; // Vertical
    } else {
        vehicle = (SDL_Rect){x - VEHICLE_HEIGHT, y - VEHICLE_WIDTH/2,
                            VEHICLE_HEIGHT * 1.5, VEHICLE_WIDTH}; // Horizontal
    }

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &vehicle);
}

void drawTrafficLight(int x, int y, int state) {
    // Draw traffic light housing
    SDL_SetRenderDrawColor(renderer, 70, 70, 70, 255);
    SDL_Rect housing = {x, y, 30, 60};
    SDL_RenderFillRect(renderer, &housing);

    // Draw light
    SDL_SetRenderDrawColor(renderer,
        state == STATE_RED ? 255 : 40,
        state == STATE_GREEN ? 255 : 40,
        40, 255);
    SDL_Rect light = {x + 5, y + 5, 20, 20};
    SDL_RenderFillRect(renderer, &light);
}

void getLanePosition(const char* lane, int* x, int* y, int vehicleIndex) {
    const int spacing = 40; // Space between vehicles

    if (strcmp(lane, "AL1") == 0 || strcmp(lane, "AL2") == 0) {
        *x = WINDOW_WIDTH/2 - (strcmp(lane, "AL1") == 0 ? LANE_WIDTH : 0);
        *y = WINDOW_HEIGHT/4 + vehicleIndex * spacing;
    } else if (strcmp(lane, "BL1") == 0) {
        *x = WINDOW_WIDTH*3/4 + vehicleIndex * spacing;
        *y = WINDOW_HEIGHT/2 - LANE_WIDTH;
    } else if (strcmp(lane, "CL1") == 0) {
        *x = WINDOW_WIDTH/2 + LANE_WIDTH;
        *y = WINDOW_HEIGHT*3/4 - vehicleIndex * spacing;
    } else if (strcmp(lane, "DL1") == 0) {
        *x = WINDOW_WIDTH/4 - vehicleIndex * spacing;
        *y = WINDOW_HEIGHT/2 + LANE_WIDTH;
    }
}

void updateTrafficLights(void) {
    static time_t lastUpdate = 0;
    time_t now = time(NULL);

    if (now - lastUpdate >= 5) { // Update every 5 seconds
        // Check priority lane (AL2)
        if (laneQueues[4].count > 10) {
            // Priority mode: AL2 gets green, others red
            for (int i = 0; i < 5; i++) {
                laneQueues[i].light_state = (i == 4) ? STATE_GREEN : STATE_RED;
            }
        } else {
            // Normal rotation mode
            static int currentGreen = 0;
            for (int i = 0; i < 4; i++) {
                laneQueues[i].light_state = (i == currentGreen) ? STATE_GREEN : STATE_RED;
            }
            currentGreen = (currentGreen + 1) % 4;
        }
        lastUpdate = now;
    }
}

void processVehicles(void) {
    for (int i = 0; i < 5; i++) {
        if (laneQueues[i].light_state == STATE_GREEN && laneQueues[i].count > 0) {
            Vehicle vehicle;
            dequeueVehicle(&laneQueues[i], &vehicle);
            printf("Vehicle %s exited from lane %s\n", vehicle.vehicle_number, vehicle.lane);
        }
    }
}

void* networkThread(void* arg) {
    (void)arg;
    int server_fd, client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        return NULL;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        return NULL;
    }

    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        close(server_fd);
        return NULL;
    }

    printf("Server listening on port %d\n", PORT);

    while (1) {
        if ((client_fd = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }

        printf("Client connected\n");

        while (1) {
            int bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
            if (bytes_read <= 0) break;

            buffer[bytes_read] = '\0';
            Vehicle vehicle;
            char lane[4];

            if (sscanf(buffer, "%[^:]:%s", vehicle.vehicle_number, lane) == 2) {
                strcpy(vehicle.lane, lane);
                vehicle.arrival_time = time(NULL);

                for (int i = 0; i < 5; i++) {
                    if (strcmp(lane, LANE_NAMES[i]) == 0) {
                        enqueueVehicle(&laneQueues[i], vehicle);
                        printf("Vehicle %s added to lane %s\n", vehicle.vehicle_number, lane);
                        break;
                    }
                }
            }
        }

        close(client_fd);
        printf("Client disconnected\n");
    }
}

void updateAndRender(void) {
    drawRoads();

    // Update and draw traffic lights
    updateTrafficLights();
    for (int i = 0; i < 5; i++) {
        int x = 0, y = 0;
        getLanePosition(LANE_NAMES[i], &x, &y, 0);
        drawTrafficLight(x - 40, y, laneQueues[i].light_state);
    }

    // Draw vehicles
    for (int i = 0; i < 5; i++) {
        pthread_mutex_lock(&laneQueues[i].mutex);
        for (int j = 0; j < laneQueues[i].count; j++) {
            int idx = (laneQueues[i].front + j) % MAX_QUEUE_SIZE;
            Vehicle vehicle = laneQueues[i].vehicles[idx];

            // Calculate vehicle color based on wait time
            time_t now = time(NULL);
            int wait_time = (int)(now - vehicle.arrival_time);
            SDL_Color color = {0, 0, 255, 255}; // Default blue

            if (wait_time > 30) {
                color = (SDL_Color){255, 0, 0, 255}; // Red for long wait
            } else if (wait_time > 15) {
                color = (SDL_Color){255, 165, 0, 255}; // Orange for medium wait
            }

            int x, y;
            getLanePosition(LANE_NAMES[i], &x, &y, j);
            drawVehicle(x, y, color, LANE_NAMES[i]);
        }
        pthread_mutex_unlock(&laneQueues[i].mutex);
    }

    processVehicles();
    SDL_RenderPresent(renderer);
}

void cleanup(void) {
    if (font) TTF_CloseFont(font);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
}

int main(void) {
    // Initialize SDL and queues
    if (!initSDL()) {
        return 1;
    }

    // Initialize queues
    for (int i = 0; i < 5; i++) {
        initVehicleQueue(&laneQueues[i]);
    }

    // Start network thread
    pthread_t network_thread;
    if (pthread_create(&network_thread, NULL, networkThread, NULL) != 0) {
        perror("Failed to create network thread");
        cleanup();
        return 1;
    }

    // Main loop
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        updateAndRender();
        SDL_Delay(16); // Cap at ~60 FPS
    }

    cleanup();
    return 0;
}