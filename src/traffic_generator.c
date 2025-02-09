#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

//#define SERVER_IP "0.0.0.0" // for all network available
#define SERVER_IP "127.0.0.1" //local host
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
    );
}       //sprintf is used to format the output and store it in buffer.-------------------------

// Generate a random lane
void generateLane(char* buffer) {
    static const char lanes[] = {"AL1", "BL1", "CL1", "DL1", "AL2"};
    strcpy(buffer, lanes[rand() % 5]); // Only incoming lanes (L1)
    
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
        char lane[4];
        generateVehicleNumber(vehicle);
        generateLane(lane);

        //Format: vehicle_number:lane
        snprintf(buffer, BUFFER_SIZE, "%s:%s", vehicle, lane);
        // Send message
        ssize_t sent_bytes = send(sock, buffer, strlen(buffer), 0);
    if (sent_bytes < 0) {
        perror("Failed to send");
        break;
    }
    printf("Sent vehicle: %s to lane: %s\n", vehicle, lane);

        //random delay between 1-3 seconds
        sleep(1+(rand() % 3));
    }

    close(sock);
    return 0;
}
