#include <stdio.h>
#include <pthread.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>

#define STRING_MAX		256

struct GameEnvironment{
    char* room_folder;
    char* rooms[7];
    int connections[7][7];
    int size;
    int start;
    int finish;
    int steps;
    int victory_path[100];
};

void loadEnvironment(struct GameEnvironment* game);
void getRoomFolder(char*);
void loadFromFiles(struct GameEnvironment* game);
void initializeEnvironment(struct GameEnvironment* game);
void freeEnvironment(struct GameEnvironment* game);
void writeCurrentTime();
void readCurrentTime();

int main(){

    struct GameEnvironment myGame;
    loadEnvironment(&myGame);           // initialize game environment variables

    /* Multi thread initialization */
    pthread_mutex_t myMutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&myMutex);
    pthread_t time_thread;
    //pthread_attr_t tattr;
    int result_code;
    /* start thread to write current time to file */
    result_code = pthread_create(&time_thread, NULL, (void*)writeCurrentTime, &myMutex);

    /* Variables used in game loop*/
    char p_connections_line[STRING_MAX];
    int current_room = myGame.start;
    myGame.victory_path[0] = current_room;
    int time_called = 0;
    size_t bufferSize = 0;
    char* lineEntered = NULL;
    int numCharEntered = -5;
    int nextRoom = -1;

    /** Game **/

    /* Loop game until user reaches the finish */
    while(current_room != myGame.finish){
        bufferSize = 0;
        lineEntered = NULL;
        numCharEntered = -5;
        nextRoom = -1;

        /* If time called in previous iteration, don't show current location */
        if(time_called == 0){
            //Display Current Location
            printf("\nCURRENT LOCATION: %s\n", myGame.rooms[current_room]);
            memset(p_connections_line, '\0', STRING_MAX);
            strcpy(p_connections_line, "POSSIBLE CONNECTIONS: ");

            //Display possible connections
            int i;
            /* Build sting of connection names: "ROOM_NAME, ROOM_NAME," etc */
            for(i = 0; i < myGame.size; i++){
                if(myGame.connections[current_room][i] == 1){
                    strcat(p_connections_line, myGame.rooms[i]);
                    strcat(p_connections_line, ", ");
                }
            }
            /* Replace last two characters " ," in string with a ".\n" */
            int last_pos = strlen(p_connections_line) - 1;
            p_connections_line[last_pos] = '\n';
            p_connections_line[last_pos-1] = '.';
            printf("%s", p_connections_line);   // string ready for print

        }
        /* Reset time flag */
        time_called = 0;

        /* Ask user for next room */
        printf("WHERE TO? >");
        numCharEntered = getline(&lineEntered, &bufferSize, stdin);

        /* Remove newline character */
        if(lineEntered[numCharEntered - 1] == '\n'){
            lineEntered[numCharEntered - 1] = '\0';
        }

        /* First check if user input is asking for time */
        if(strcmp(lineEntered, "time") == 0){
            pthread_mutex_unlock(&myMutex);     //unlock mutex to allow time thread to continue
            pthread_join(time_thread, NULL);    //have main thread wait for time thread
            pthread_mutex_lock(&myMutex);       //re-gain control of mutex
            /* Re-start time thread */
            result_code = pthread_create(&time_thread, NULL, (void*)writeCurrentTime, &myMutex);
            readCurrentTime();  // read time from file
            time_called = 1;    // time flag to prevent display of current location in next iteration
        } else {
            /* Check if user input is a room name */
            int j;
            for(j = 0; j < myGame.size; j++){
                if(strcmp(lineEntered, myGame.rooms[j]) == 0){
                    //Check if that room is a connection
                    if(myGame.connections[current_room][j] == 1){
                        current_room = j;
                        myGame.steps++;
                        /* Just in case user steps exceed size of array that tracks steps */
                        if(myGame.steps >= 100){
                            printf("\nYOU ARE SO BAD AT THIS! MERCY RULE INVOKED!\n");
                        } else {
                            myGame.victory_path[myGame.steps] = current_room;       // track room before leaving it
                        }
                    } else {
                        /* If room is a valid name, but not connected */
                        printf("\n%s IS NOT CONNECTED to %s\n", lineEntered, myGame.rooms[current_room]);
                        //nextRoom = -2;
                    }
                    j =  myGame.size;
                } else if(j == myGame.size - 1) {pthread_detach(time_thread);
                    printf("\nHUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN.\n");    // room name not valid
                }
            }
        }
        free(lineEntered);
    }

    /* Game Over */
    if(myGame.steps < 100){
        printf("\nYOU HAVE FOUND THE ROOM. CONGRATULATIONS!\n");
        printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", myGame.steps);
    } else {
        printf("\nYOUR MINDLESS WANDERING WAS:\n");         // if user took 100 or more steps
    }

    /* Loop through victory_path array for indexes which refer to the room names (myGame.rooms) */
    int i = 0;
    while(i < 100 && myGame.victory_path[i] != -1){
        printf("%s\n", myGame.rooms[myGame.victory_path[i]]);
        i++;
    }

    freeEnvironment(&myGame);           // free up dynamic memory
    //pthread_mutex_unlock(&myMutex);     //unlock mutex to allow time thread to continue (else memory leaks)
    //pthread_join(time_thread, NULL);    //have main thread wait for time thread
    pthread_cancel(time_thread);
    pthread_mutex_destroy(&myMutex);    // destroy mutex

    /*
        Note: currentTime.txt will have the time stamp of when this program terminated.
        This is due to letting the 2nd thread execute in full at the end of the program.
        Otherwise, there are memory leak issues.
        Tried to resolve this on Piazza, but could not:
        https://piazza.com/class/ixhzh3rn2la6vk?cid=245
    */
    return 0;
}


void initializeEnvironment(struct GameEnvironment* game){
    /* Dynamically allocate room names */
    int i = 0;
    while(i < 7){
        game->rooms[i] = malloc(30 * sizeof(char));
        assert(game->rooms[i] != 0);
        memset(game->rooms[i], '\0', 30);
        i++;
    }

    /* Dynamically allocate room folder */
    game->room_folder = malloc(40 * sizeof(char));
    assert(game->room_folder != 0);
    memset(game->room_folder, '\0', 40);

    /* Set 2d array elements, size, and steps to 0 */
    memset(game->connections, 0, sizeof(game->connections[0][0]) * 7 * 7);
    memset(game->victory_path, -1, sizeof(game->victory_path[0]) * 100);
    game->size = 0;
    game->steps = 0;
}


void freeEnvironment(struct GameEnvironment* game){
    /* Free dynamic memory in game environment */
    int i = 0;
    while(i < 7){
        free(game->rooms[i]);
        i++;
    }
    free(game->room_folder);
}


void loadEnvironment(struct GameEnvironment* game){//
    /* call functions to create game environment */
    initializeEnvironment(game);
    getRoomFolder(game->room_folder);
    loadFromFiles(game);
}


void getRoomFolder(char* folder_path){
    struct stat st;
    struct dirent *dir_entry;
    DIR *directory = opendir(".");

    if (directory == NULL){
        fprintf(stderr, "Could not open directory\n");
        exit(1);
    }

    int newest = 0;
    int current;
    /* Loop through each item in directory */
    while (dir_entry = readdir(directory)){
        /* If item in directory matches naming scheme of room folders, then check modified time */
        if(dir_entry->d_ino != 0 && strstr(dir_entry->d_name, "krusej.rooms.") != NULL){
            stat(dir_entry->d_name, &st);
            current = st.st_mtime;
            /* Check and set most recent modified folder */
            if(newest < current){
                newest = current;
                sprintf(folder_path, dir_entry->d_name);
            }
        }
    }
    strcat(folder_path, "/");
    closedir(directory);
}


void loadFromFiles(struct GameEnvironment* game){
    struct dirent *dir_entry;
    DIR *directory = opendir(game->room_folder);

    char file_path[STRING_MAX];

    int file_descriptor;
    ssize_t nread, nwritten;
    char readBuffer[STRING_MAX];

    if (directory == NULL){
        fprintf(stderr, "Could not open file\n");
        exit(1);
    }
    /* loop through directory to find files with '_' in the name */
    while (dir_entry = readdir(directory)){
        if(dir_entry->d_ino != 0 && strstr(dir_entry->d_name, "_") != NULL){
            memset(file_path, '\0', STRING_MAX);
            strcpy(file_path, game->room_folder);
            strcat(file_path, dir_entry->d_name);

            file_descriptor = open(file_path, O_RDONLY);

            if (file_descriptor == -1){
                printf("Hull breach - open() failed on \"%s\"\n", file_path);
                perror("In loadFromFiles()");
            }

            /* read file, parse file */
            memset(readBuffer, '\0', sizeof(readBuffer)); // Clear out the array before using it
            lseek(file_descriptor, 0, SEEK_SET); // Reset the file pointer to the beginning of the file
            nread = read(file_descriptor, readBuffer, sizeof(readBuffer));


            char* token = strtok(readBuffer, ":");  //ignore the first parse
            token = strtok(NULL, " \n");            // room name

            /* check if room name already exists in room array */
            int j = 0;
            int i = 0;
            for(j = 0; j < game->size; j++){
                if(strcmp(game->rooms[j], token) == 0){
                    i=j;
                    j = game->size;
                } else {
                    i++;

                }
            }
            if(i == game->size){
                sprintf(game->rooms[i], token);     // room needs to be added to array
                game->size++;
            }
            /* Get room connections from file until the type of room is reached */
            while(strcmp(token, "START_ROOM") != 0 && strcmp(token, "MID_ROOM") != 0 && strcmp(token, "END_ROOM") != 0){
                char temp[STRING_MAX];
                memset(temp, '\0', STRING_MAX);

                token = strtok(NULL, ":");
                strcpy(temp, token);

                token = strtok(NULL, " \n");

                if(strstr(temp, "CONNECTION") != NULL){
                        /**
                            - check if connection name is in rooms
                            - if yes:
                                assign j to room index
                            - if no:
                                - add name to rooms[size].
                                - assign j to size.
                                - increment size
                            - Assign connections[i][j] to 1
                        **/
                        int j = 0;
                        int k = 0;
                        for(k = 0; k < game->size; k++){
                            if(strcmp(game->rooms[k], token) == 0){
                                //printf("found room!\n");
                                j=k;
                                k = game->size;
                            } else {
                                j++;
                            }
                        }
                        if(j == game->size){
                            sprintf(game->rooms[j], token);
                            game->size++;
                        }
                        game->connections[i][j] = 1;
                }
                /* set start and finish rooms */
                else if(strstr(temp, "TYPE") != NULL){
                    if(strcmp(token, "START_ROOM") == 0){
                        game->start = i;
                    }
                    else if(strcmp(token, "END_ROOM") == 0){
                        game->finish = i;
                    }
                }
            }
        }
    }
    closedir(directory);
}


void writeCurrentTime(pthread_mutex_t* a_mutex){
    time_t rawtime;
    struct tm* timeinfo;
    ssize_t nwritten;
    int file_descriptor;

    char buffer[STRING_MAX];
    memset(buffer, '\0', STRING_MAX);
    char file_path[STRING_MAX];
    memset(file_path, '\0', STRING_MAX);
    strcpy(file_path, "currentTime.txt");

    pthread_mutex_lock(a_mutex);            // try to lock mutex, else pause thread

    /* open file for writing, then write current time */
    file_descriptor = open(file_path, O_WRONLY | O_TRUNC | O_CREAT, 0600);

    if(file_descriptor < 0){
        printf("Could not open/create file\n");
        exit(1);
    }

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer, 50, "%I:%M%P, %A, %B %d, %Y", timeinfo);
    nwritten = write(file_descriptor, buffer, strlen(buffer) * sizeof(char));

    close(file_descriptor);

    pthread_mutex_unlock(a_mutex);
}


void readCurrentTime(){
    char buffer[50];
    int file_descriptor;
    ssize_t nread;

    char file_path[STRING_MAX];
    memset(file_path, '\0', STRING_MAX);
    strcpy(file_path, "currentTime.txt");

    file_descriptor = open(file_path, O_RDONLY);

    if (file_descriptor == -1){
        printf("Hull breach - open() failed on \"%s\"\n", file_path);
        perror("In loadFromFiles()");
    }

    memset(buffer, '\0', sizeof(buffer)); // Clear out the array before using it
    lseek(file_descriptor, 0, SEEK_SET); // Reset the file pointer to the beginning of the file
    nread = read(file_descriptor, buffer, sizeof(buffer));

    close(file_descriptor);

    printf("\n%s\n\n", buffer);
}
