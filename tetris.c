#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <ncurses.h>
#include <arpa/inet.h>
#include <string.h>
#include <Kernel/sys/_endian.h>
#include <sys/_endian.h>

#define PORT 12345
#define MAX_PENDING 5
#define MAX_LINE 256

#define ROWS 20 // you can change height and width of table with ROWS and COLS 
#define COLS 15
#define TRUE 1
#define FALSE 0
#define PACKET_SIZE 1000

char Table[ROWS][COLS] = {0};
int score = 0;
char GameOn = TRUE;
suseconds_t timer = 400000; // decrease this to make it faster
int decrease = 1000;

typedef struct {
    char **array;
    int width, row, col;
} Shape;
Shape current;

const Shape ShapesArray[7]= {
	{(char *[]){(char []){0,1,1},(char []){1,1,0}, (char []){0,0,0}}, 3},                           //S shape     
	{(char *[]){(char []){1,1,0},(char []){0,1,1}, (char []){0,0,0}}, 3},                           //Z shape     
	{(char *[]){(char []){0,1,0},(char []){1,1,1}, (char []){0,0,0}}, 3},                           //T shape     
	{(char *[]){(char []){0,0,1},(char []){1,1,1}, (char []){0,0,0}}, 3},                           //L shape     
	{(char *[]){(char []){1,0,0},(char []){1,1,1}, (char []){0,0,0}}, 3},                           //flipped L shape    
	{(char *[]){(char []){1,1},(char []){1,1}}, 2},                                                 //square shape
	{(char *[]){(char []){0,0,0,0}, (char []){1,1,1,1}, (char []){0,0,0,0}, (char []){0,0,0,0}}, 4} //long bar shape
	// you can add any shape like it's done above. Don't be naughty.
};

Shape CopyShape(Shape shape){
	Shape new_shape = shape;
	char **copyshape = shape.array;
	new_shape.array = (char**)malloc(new_shape.width*sizeof(char*));
    int i, j;
    for(i = 0; i < new_shape.width; i++){
		new_shape.array[i] = (char*)malloc(new_shape.width*sizeof(char));
		for(j=0; j < new_shape.width; j++) {
			new_shape.array[i][j] = copyshape[i][j];
		}
    }
    return new_shape;
}

void DeleteShape(Shape shape){
    int i;
    for(i = 0; i < shape.width; i++){
		free(shape.array[i]);
    }
    free(shape.array);
}

int CheckPosition(Shape shape){ //Check the position of the copied shape
	char **array = shape.array;
	int i, j;
	for(i = 0; i < shape.width;i++) {
		for(j = 0; j < shape.width ;j++){
			if((shape.col+j < 0 || shape.col+j >= COLS || shape.row+i >= ROWS)){ //Out of borders
				if(array[i][j]) //but is it just a phantom?
					return FALSE;
				
			}
			else if(Table[shape.row+i][shape.col+j] && array[i][j])
				return FALSE;
		}
	}
	return TRUE;
}

void SetNewRandomShape(){ //updates [current] with new shape
	Shape new_shape = CopyShape(ShapesArray[rand()%7]);

    new_shape.col = rand()%(COLS-new_shape.width+1);
    new_shape.row = 0;
    DeleteShape(current);
	current = new_shape;
	if(!CheckPosition(current)){
		GameOn = FALSE;
	}
}

void RotateShape(Shape shape){ //rotates clockwise
	Shape temp = CopyShape(shape);
	int i, j, k, width;
	width = shape.width;
	for(i = 0; i < width ; i++){
		for(j = 0, k = width-1; j < width ; j++, k--){
				shape.array[i][j] = temp.array[k][i];
		}
	}
	DeleteShape(temp);
}

void WriteToTable(){
	int i, j;
	for(i = 0; i < current.width ;i++){
		for(j = 0; j < current.width ; j++){
			if(current.array[i][j])
				Table[current.row+i][current.col+j] = current.array[i][j];
		}
	}
}

void RemoveFullRowsAndUpdateScore(){
	int i, j, sum, count=0;
	for(i=0;i<ROWS;i++){
		sum = 0;
		for(j=0;j< COLS;j++) {
			sum+=Table[i][j];
		}
		if(sum==COLS){
			count++;
			int l, k;
			for(k = i;k >=1;k--)
				for(l=0;l<COLS;l++)
					Table[k][l]=Table[k-1][l];
			for(l=0;l<COLS;l++)
				Table[k][l]=0;
			timer-=decrease--;
		}
	}
	score += 100*count;
}

void PrintTable(){
	char Buffer[ROWS][COLS] = {0};
	int i, j;
	for(i = 0; i < current.width ;i++){
		for(j = 0; j < current.width ; j++){
			if(current.array[i][j])
				Buffer[current.row+i][current.col+j] = current.array[i][j];
		}
	}
	clear();
	for(i=0; i<COLS-9; i++)
		printw(" ");
	printw("Covid Tetris\n");
	for(i = 0; i < ROWS ;i++){
		for(j = 0; j < COLS ; j++){
			printw("%c ", (Table[i][j] + Buffer[i][j])? '#': '.');
		}
		printw("\n");
	}
	printw("\nScore: %d\n", score);
}

void ManipulateCurrent(int action){
	Shape temp = CopyShape(current);
	switch(action){
		case 's':
			temp.row++;  //move down
			if(CheckPosition(temp))
				current.row++;
			else {
				WriteToTable();
				RemoveFullRowsAndUpdateScore();
                SetNewRandomShape();
			}
			break;
		case 'd':
			temp.col++;  //move right
			if(CheckPosition(temp))
				current.col++;
			break;
		case 'a':
			temp.col--;  //move left
			if(CheckPosition(temp))
				current.col--;
			break;
		case 'w':
			RotateShape(temp); // rotate clockwise
			if(CheckPosition(temp))
				RotateShape(current);
			break;
	}
	DeleteShape(temp);
	PrintTable();
}

struct timeval before_now, now;
int hasToUpdate(){
	return ((suseconds_t)(now.tv_sec*1000000 + now.tv_usec) -((suseconds_t)before_now.tv_sec*1000000 + before_now.tv_usec)) > timer;
}

// Function to process received data and move the game
void ProcessReceivedData(char action) {
    switch (action) {
        case 's':
        case 'd':
        case 'a':
        case 'w':
            ManipulateCurrent(action); // Call your existing game logic function
            break;
        case 'q':
            GameOn = FALSE; // Terminate the game
            break;
        default:
            // Handle invalid input
            break;
    }
}
// Define a data structure to hold packet data
typedef struct {
    char payload[1000]; // An array to store the packet's data payload
} DataPacket;

// Define a data structure to represent a frame
typedef struct {
    int frameType; // The type of frame: ACK=0, SEQ=1, FIN=2
    int sequenceNum; // Sequence number of the frame
    int acknowledgmentNum; // Acknowledgment number of the frame
    DataPacket packetData; // The data packet carried within the frame
} TransmissionFrame;

double packetLossProbability = 0.1;

// Function to simulate packet loss based on the set probability
int simulatePacketLoss(void) {
    return ((double)rand() / (double)RAND_MAX) < packetLossProbability;
}
int main() {
    srand(time(NULL)); // Initialize random number generator with the current time

    int playerScore = 0;
    int inputChar;

    // Initialize screen for game display
    initscr();
    cbreak(); // Disable line buffering
    noecho(); // Do not echo pressed keys
    nodelay(stdscr, TRUE); // Non-blocking input handling

    struct timeval start_time;
    gettimeofday(&start_time, NULL);
    timeout(1);

    InitRandomShape();    
    DisplayGrid();

    // Setup UDP socket
    int socket_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;
    char dataBuffer[MAX_BUFFER_SIZE];
    int bytesRead;

    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        perror("Failed to open socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT_NUM);

    if (bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }

    client_addr_len = sizeof(client_addr);

    int ackFlag = 0;
    int lastSeqNum = 0;

    while (GameIsActive) {
        // Handle keyboard input
        if ((inputChar = getch()) != ERR) {
            ProcessInput(inputChar);
        }

        // Network communication
        if (!simulate_packet_loss()) {
            memset(dataBuffer, 0, MAX_BUFFER_SIZE);
            bytesRead = recvfrom(socket_fd, dataBuffer, MAX_BUFFER_SIZE-1, 0, (struct sockaddr *)&client_addr, &client_addr_len);
            if (bytesRead < 0) {
                perror("Socket read error");
                exit(EXIT_FAILURE);
            }

            if (dataBuffer[0] == 'A') { // Check for ACK
                ackFlag = 1;
            } else {
                ProcessInput(dataBuffer[0]); // Process command from the client

                memset(dataBuffer, 0, MAX_BUFFER_SIZE);
                dataBuffer[0] = 'A'; // Prepare ACK
                sendto(socket_fd, dataBuffer, 1, 0, (struct sockaddr *)&client_addr, client_addr_len);
            }
        } else {
            printf("[-] Simulating packet loss\n");
            ackFlag = 0;
        }

        struct timeval current_time;
        gettimeofday(&current_time, NULL);
        if (time_to_update()) {
            ProcessInput('s');
            gettimeofday(&start_time, NULL);
        }

        if (!ackFlag) {
            memset(dataBuffer, 0, MAX_BUFFER_SIZE);
            dataBuffer[0] = inputChar; // Resend previous command if ACK not received
            sendto(socket_fd, dataBuffer, 1, 0, (struct sockaddr *)&client_addr, client_addr_len);
        }
    }

    endwin(); // Close the graphical window

    // Print game state and score
    int i, j;
    for (i = 0; i < ROWS; i++) {
        for (j = 0; j < COLS; j++) {
            printf("%c ", Table[i][j] ? '#' : '.');
        }
        printf("\n");
    }
    printf("\nGame over!\n");
    printf("\nScore: %d\n", playerScore);
    return 0;
}