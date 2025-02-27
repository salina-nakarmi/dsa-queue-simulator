#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

//#define SERVER_IP "0.0.0.0" // for all network available
#define SERVER_IP "127.0.0.1" 
#define PORT 8000
#define BUFFER_SIZE 100

// Generate a random vehicle number
void generateVehicleNumber(char* buffer) {
    static const char* states[] = {"KA", "MH", "DL", "UP", "TN"};
    sprintf(buffer, "%s%02d%c%c%04d", 
        states[rand() % 5],           // Random state code
        rand() % 100,                 // District code
        'A' + rand() % 26,           // Random letter
        'A' + rand() % 26,           // Random letter
        rand() % 10000               // Random number
            // District code            // Random number
    );
}       //sprintf is used to format the output and store it in buffer.-------------------------

// Generate a random lane (A1, B1, C1, D1, or AL2)
void generateLane(char* lane) {
    // Define possible lanes including the new lanes
    static const char* lanes[] = {"A1", "B1", "C1", "D1", "A2", "B2", "C2", "D2"};
    
    // Randomly select a lane
    int lane_index = rand() % 8;
    strcpy(lane, lanes[lane_index]);
}

// Generate a vehicle from a specific source lane and determine its target lane
void generateVehicleRoute(char* source_lane, char* target_lane) {
    // Define our source lanes (the generating lanes)
    static const char* source_lanes[] = {"A3", "B3", "C3", "D3", "A2", "B2", "C2", "D2"};
    
    // Randomly select a source lane
    int lane_index = rand() % 8;
    strcpy(source_lane, source_lanes[lane_index]);
    
    // Determine target lane based on source lane
    if (strcmp(source_lane, "A3") == 0) {
        strcpy(target_lane, "B1");        // A3 vehicles go to B1
    } else if (strcmp(source_lane, "B3") == 0) {
        strcpy(target_lane, "C1");        // B3 vehicles go to C1
    } else if (strcmp(source_lane, "C3") == 0) {
        strcpy(target_lane, "D1");        // C3 vehicles go to D1
    } else if (strcmp(source_lane, "D3") == 0) {
        strcpy(target_lane, "A1");        // D3 vehicles go to A1
    } else if (strcmp(source_lane, "A2") == 0) {
        // AL2 can go to B1 (right) or C1 (straight)
        strcpy(target_lane, (rand() % 2 == 0) ? "B1" : "C1");
    } else if (strcmp(source_lane, "BL2") == 0) {
        // BL2 can go to C1 (right) or D1 (straight)
        strcpy(target_lane, (rand() % 2 == 0) ? "C1" : "D1");
    } else if (strcmp(source_lane, "CL2") == 0) {
        // CL2 can go to D1 (right) or A1 (straight)
        strcpy(target_lane, (rand() % 2 == 0) ? "D1" : "A1");
    } else if (strcmp(source_lane, "DL2") == 0) {
        // DL2 can go to A1 (right) or B1 (straight)
        strcpy(target_lane, (rand() % 2 == 0) ? "A1" : "B1");
    }
}

int main() {
    int sock;
    struct sockaddr_in server_address;
    char buffer[BUFFER_SIZE];

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &server_address.sin_addr) <= 0) {
        perror("Invalid address");
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    printf("Connected to simulator...\n");
    srand(time(NULL));

    while (1) {
        char vehicle[15];
        char source_lane[4];
        char target_lane[4];
        generateVehicleNumber(vehicle);
        generateVehicleRoute(source_lane, target_lane);


          // Format: vehicle_number:target_lane
        // We're sending the vehicle to the target lane based on source lane
        // Change this in traffic_generator.c
        snprintf(buffer, BUFFER_SIZE, "%s:%s", vehicle, source_lane);  // Send source lane
        // Send message
        ssize_t sent_bytes = send(sock, buffer, strlen(buffer), 0);
    if (sent_bytes < 0) {
        perror("Failed to send");
        break;
    }
     printf("Vehicle from %s: %s sent to lane: %s\n", source_lane, vehicle, target_lane);

    usleep((100000 + (rand() % 400000)));  // Delay between 0.1 to 0.5 seconds
    }

    close(sock);
    return 0;
}
