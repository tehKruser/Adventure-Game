#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define STRING_MAX		256


int GetRandomNum(int min, int max){
    int range = max - min + 1;
    int randNum = rand() % range + min;
    return randNum;
}

void swap(int *A, int a, int b){
    int temp = A[a];
    A[a] = A[b];
    A[b] = temp;
}


void main() {
    srand(time(NULL));

    /* Get process ID */
    int myPID = getpid();
    char myThreadIDs[STRING_MAX];
    memset(myThreadIDs, '\0', STRING_MAX);
    sprintf(myThreadIDs, "%u", myPID);

    /* Directory path for room files */
    char base_dir[STRING_MAX] = "krusej.rooms.";

    strcat(base_dir, myThreadIDs);

    ssize_t nread, nwritten;
    char readBuffer[32];
    memset(readBuffer, '\0', 32);

    int i = 0;
    int j = 0;
    int k = 0;

    /* Room names array */
    const char* rooms[10];
    rooms[0] = "Damnation";
    rooms[1] = "HangemHigh";
    rooms[2] = "Longest";
    rooms[3] = "Chillout";
    rooms[4] = "Wizard";
    rooms[5] = "Prisoner";
    rooms[6] = "Derelict";
    rooms[7] = "BloodGulch";
    rooms[8] = "Sidewinder";
    rooms[9] = "BattleCreek";


    /* Array to track which rooms are randomly selected (A[0...6]) */
    int rooms_selected[10];
    for(i = 0; i < 10; i++){
        rooms_selected[i] = i;
    }

    /* Randomly select 7 of the 10 rooms */
    int myRandy;
    for(k = 0; k < 7; k++){
        myRandy = GetRandomNum(k, 9);
        swap(rooms_selected, k, myRandy);
    }

    /* Randomly determine how many connections each room will have */
    int required_connections[7];
    int rand_conn_count;
    for(i = 0; i < 7; i++){
        rand_conn_count = GetRandomNum(3, 6);
        required_connections[i] = rand_conn_count;
    }

    /* Array to track each room's currently established connection value and what the connections are */
    // Details:
    // rows X columns
    // rows = rooms
    // column 0: number of established connections
    // columns 1-6: values of established connections (0 or 1)
    int established_connections[7][7];
    for(i = 0; i < 7; i++){
        established_connections[i][0] = 0;
    }

    /* Populate 2D array for connections */
    for (i = 0; i < 7; i++){
        /* For [current connections] to [required connections] of the room */
        for(j = established_connections[i][0]; j < required_connections[i]; j++){
            /* While loop to get a connection room */
            int connection_found = 0;
            int randConn;
            int connection_list[7];
            while (connection_found == 0){
                connection_found = 1;
                randConn = GetRandomNum(0, 5);
                if(randConn >= i){
                    randConn++;
                }
                for(k = 1; k <= established_connections[i][0]; k++){
                    if(randConn == established_connections[i][k])
                        connection_found = 0;
                }
                if(connection_found == 0){
                    int desired_connection_connections = established_connections[randConn][0];
                    if(desired_connection_connections >= required_connections[randConn]){
                        connection_found = 0;
                    }
                }
            }
            // Update current connections for i
            established_connections[i][0] = established_connections[i][0] + 1;
            established_connections[i][established_connections[i][0]] = randConn;

            // Update connection that i connected to
            established_connections[randConn][0] = established_connections[randConn][0] + 1;
            established_connections[randConn][established_connections[randConn][0]] = i;
        }
    }

    /* Create directory for current process */
    struct stat st = {0};
    if(stat(base_dir, &st) == -1){
        mkdir(base_dir, 0700);
    }

    /* Create files for the 7 rooms */
    int file_descriptor;
    for(i=0; i<7; i++){
        char file_path[STRING_MAX];
        memset(file_path, '\0', STRING_MAX);
        strcpy(file_path, base_dir);
        strcat(file_path, "/");
        strcat(file_path, rooms[rooms_selected[i]]);
        strcat(file_path, "_room");

        file_descriptor = open(file_path, O_RDWR | O_CREAT, 0600);

        if(file_descriptor < 0){
            printf("Could not open/create file\n");
            exit(1);
        }

        /* Build string to write room name */
        char line_1[STRING_MAX];
        memset(line_1, '\0', STRING_MAX);
        strcpy(line_1, "ROOM NAME: ");
        strcat(line_1, rooms[rooms_selected[i]]);
        strcat(line_1, "\n");

        nwritten = write(file_descriptor, line_1, strlen(line_1) * sizeof(char));

        /* loop through connections 2D array to write which rooms are connected */
        for(j = 1; j <= established_connections[i][0]; j++){
            char j_str[8];
            memset(j_str, '\0', 8);
            sprintf(j_str, "%d", j);
            char connection_line[STRING_MAX] = "";
            memset(connection_line, '\0', STRING_MAX);

            /* build string for writing */
            strcpy(connection_line, "CONNECTION ");
            strcat(connection_line, j_str);
            strcat(connection_line, ": ");
            strcat(connection_line, rooms[rooms_selected[established_connections[i][j]]]);
            strcat(connection_line, "\n");

            nwritten = write(file_descriptor, connection_line, strlen(connection_line) * sizeof(char));
        }

        /* build string for the type of room, then write to file */
        char roomtype_line[STRING_MAX];
        memset(roomtype_line, '\0', STRING_MAX);
        if(i == 0){
            strcpy(roomtype_line, "ROOM TYPE: START_ROOM\n");
        } else if (i == 6) {
            strcpy(roomtype_line, "ROOM TYPE: END_ROOM\n");
        } else {
            strcpy(roomtype_line, "ROOM TYPE: MID_ROOM\n");
        }

        nwritten = write(file_descriptor, roomtype_line, strlen(roomtype_line) * sizeof(char));

        memset(readBuffer, '\0', sizeof(readBuffer)); // Clear out the array before using it
        lseek(file_descriptor, 0, SEEK_SET); // Reset the file pointer to the beginning of the file

        close(file_descriptor);
    }
}
