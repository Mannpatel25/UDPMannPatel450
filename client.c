#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <termios.h>
#include <fcntl.h>
#include <curses.h>


#define PORT 12345
#define SERVER_IP "127.0.0.1"
#define MAX_LINE 256
#define PACKET_SIZE 1000
#define TIMEOUT_SEC 1
#define TIMEOUT_USEC 100

char isActive = 1; // Control the main game loop
int inputKey;
int totalInitialPackets = 0;
int totalResendPackets = 0;
int totalLostPackets = 0;
int totalSuccessPackets = 0;
int totalAcks = 0;
int totalTimeouts = 0;

void show_statistics() {
    printf("\n=== Client Performance Metrics ===\n");
    printf("Initial Packets Sent: %d\n", totalInitialPackets);
    printf("Total Packets Sent (Initial + Resent): %d\n", totalInitialPackets + totalResendPackets);
    printf("Lost Packets: %d\n", totalLostPackets);
    printf("Delivered Packets: %d\n", totalSuccessPackets);
    printf("Received ACKs: %d\n", totalAcks);
    printf("Timeout Events: %d\n", totalTimeouts);
}

int main() {
    srand(time(NULL)); // Initialize random number generator

    initscr();
    cbreak();
    noecho();
    nodelay(stdscr, TRUE); // Non-blocking key reading

    int udpSocket;
    struct sockaddr_in serverAddress;
    struct hostent *serverHost;

    udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket < 0) {
        perror("Cannot create socket");
        endwin();
        exit(EXIT_FAILURE);
    }

    serverHost = gethostbyname("localhost");
    if (!serverHost) {
        fprintf(stderr, "Host not found\n");
        endwin();
        exit(EXIT_FAILURE);
    }

    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    memcpy(&serverAddress.sin_addr.s_addr, serverHost->h_addr, serverHost->h_length);
    serverAddress.sin_port = htons(SERVER_PORT);

    while (isActive) {
        char txBuffer[MAX_BUFFER] = {0};
        if ((inputKey = getch()) != ERR) {
            snprintf(txBuffer, sizeof(txBuffer), "Key: %c", inputKey);
            totalInitialPackets++;

            int sendResult = sendto(udpSocket, txBuffer, strlen(txBuffer), 0, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
            if (sendResult < 0) {
                perror("Failed to send data");
                endwin();
                close(udpSocket);
                exit(EXIT_FAILURE);
            }

            printw("Sending packet #%d\n", totalInitialPackets);

            struct timeval selectTimeout = {5, 0}; // 5 seconds timeout
            fd_set readSet;
            FD_ZERO(&readSet);
            FD_SET(udpSocket, &readSet);

            int selectStatus = select(udpSocket + 1, &readSet, NULL, NULL, &selectTimeout);
            if (selectStatus < 0) {
                perror("Select error");
                endwin();
                exit(EXIT_FAILURE);
            } else if (selectStatus == 0) {
                totalTimeouts++;
                printw("Timeout for packet #%d\n", totalInitialPackets);
                totalResendPackets++;
                continue; // Try sending the packet again
            }

            totalAcks++;
            printw("ACK for packet #%d received\n", totalInitialPackets);
            totalSuccessPackets++;
        } else {
            sleep(1); // Throttle down when no input is detected
        }
    }

    show_statistics();
    endwin(); // Close NCurses
    close(udpSocket); // Close the network socket
    return 0;
}