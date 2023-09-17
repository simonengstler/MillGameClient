#define _POSIX_SOURCE

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/select.h>
#include "../Communication/serverCommunication.h"
#include "../Config/config.h"
#include "../Utility/errorHandling.h"
#include "../Utility/utility.h"
#include "../Communication/processCommunication.h"
#include "client.h"
#include "../Thinker/thinker.h"

#define max(x, y) (((x) >= (y)) ? (x) : (y))
#define BUF 256

struct sockaddr_in server_addr;
struct hostent *host;
void *sharedMemory;

int shmID;
int pipeFd[2];

// valgrind --leak-check=full --trace-children=yes ./sysprak-client -g 3numqm976nlmn -p 0 -c client.conf
// coolgame für samuel : ./sysprak-client -g 3numqm976nlmn -p 0 -c client.conf 
// ./sysprak-client -g 13l5isu7hmidt -p 0 -c client.conf
// ENDLESS-GAME (only for A0-A7-Loop): ./sysprak-client -g 0e22xixtuj746 -p 0

int initializeSocket() {
    int sfd;
    if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        handleError(ENOTCONN, "Socket-Connection unsuccessful");
    }
    return sfd;
}

void initializeServerAddress(configSpecification *configSpecs) {
    if ((host = gethostbyname(configSpecs->hostName)) == NULL) {  /* get host info */
        handleError(EINVAL, "Error when retrieving hostname");
    }
    memset(&server_addr, '0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(configSpecs->portNumber);
    memcpy(&server_addr.sin_addr, host->h_addr_list[0], host->h_length);
}

void initializeSharedGameInformation(sharedGameInformation *sharedGameInfo) {
    sharedGameInfo->numberOfPlayers = -1;
    sharedGameInfo->clientPlayerNumber = -1;
    sharedGameInfo->connectorPID = -1;
    sharedGameInfo->thinkerPID = -1;
    memset(sharedGameInfo->gameName, 0, 256);
    sharedGameInfo->thinkingFinished = false;
    sharedGameInfo->numberOfStonesToCapture = 0;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 8; j++) {
            sharedGameInfo->playingField[i][j] = -1;
        }
    }

}

void initializeSharedPlayerInformation(sharedPlayerInformation *sharedPlayerInfo) {
    sharedPlayerInfo->playerNumber = -1;
    sharedPlayerInfo->playerIsReady = 0;
    memset(sharedPlayerInfo->playerName, 0, 256);
}

void startThinking(int signalKey) {
    //printf("\n>>Application started thinking<<\n");
    initiateThinking(sharedMemory, pipeFd);
}

void startGame(int socketFd, sharedGameInformation *sharedGameInfo) {
    bool gameIsOver = false;

    fd_set current_sockets, ready_sockets;
    FD_ZERO(&current_sockets);
    FD_SET(pipeFd[0], &current_sockets);
    FD_SET(socketFd, &current_sockets);
    int max_socket = max(socketFd, pipeFd[0]);

    while (!gameIsOver) {
        FD_ZERO(&ready_sockets);
        ready_sockets = current_sockets;
        int retval;
        if ((retval = select(max_socket + 1, &ready_sockets, NULL, NULL, NULL)) < 0) {
            handleError(EPIPE, "Error with initializing the event listeners.");
        }

        if (FD_ISSET(socketFd, &ready_sockets)) {
            gameIsOver = readGameStatus(socketFd, sharedGameInfo);
        }
        if ((FD_ISSET(pipeFd[0], &ready_sockets)) && sharedGameInfo->thinkingFinished == true) {
            char resultOfThinking[7];
            readResultOfThinking(pipeFd[0], resultOfThinking);
            sendToServer(socketFd, "PLAY", resultOfThinking);
            FD_CLR(pipeFd[0], &ready_sockets);
            sharedGameInfo->thinkingFinished = false;
        }
    }
}

int main(int argc, char *argv[]) {
    pid_t pid;
    if ((shmID = shmget(IPC_PRIVATE, sizeof(sharedGameInformation) + sizeof(sharedPlayerInformation),
                        IPC_CREAT | 0644)) < 0) {
        handleError(ENOMEM, "Created shared memory failed.");
    }

    if (pipe(pipeFd)) {
        handleError(EPIPE, "Creating an unnamed pipe failed.");
    }

    if ((pid = fork()) < 0) {
        handleError(ESRCH, "Error with fork");
    } else if (pid == 0) { /* Connector (child) */
        close(pipeFd[1]);

        int socketFd;
        char gameId[14];
        char playerNumber[2];
        char configFileName[256];
        configSpecification configSpecs;
        sharedGameInformation *sharedGameInfo;
        sharedPlayerInformation *sharedPlayerInfoOpponent;
        sharedMemory = startProcessCommunication(shmID);
        sharedGameInfo = (sharedGameInformation *) sharedMemory;
        sharedPlayerInfoOpponent = (sharedPlayerInformation *) (sharedMemory +
                                                                sizeof(sharedGameInformation)); //in same shared Memory

        memset(configFileName, 0, 256); //initialize configFileName
        memset(playerNumber, 0, 2);
        initializeSharedGameInformation(sharedGameInfo);
        initializeSharedPlayerInformation(sharedPlayerInfoOpponent);
        initializeConfigSpecification(&configSpecs, configFileName);
        initializeServerAddress(&configSpecs);

        processArgs(argc, argv, gameId, playerNumber, configFileName);

        sharedGameInfo->connectorPID = getpid();
        sharedGameInfo->thinkerPID = getppid();

        socketFd = initializeSocket();

        if (connect(socketFd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr)) != -1) {
            printf(">>Connection successful<<\n\n");

            communicateGameSetup(socketFd, gameId, playerNumber, sharedGameInfo, sharedPlayerInfoOpponent);
            checkIfAllGameInfosAreProvided(sharedGameInfo);
            checkIfAllPlayerInfosAreProvided(sharedPlayerInfoOpponent);

            startGame(socketFd, sharedGameInfo);

            close(socketFd);
        } else {
            handleError(ENOTCONN, "Connection unsuccessful");
        }

    } else { /* Thinker (parent) */
        sharedMemory = startProcessCommunication(shmID);
        int returnValFromWaitPid;
        close(pipeFd[0]);
        struct sigaction sa;
        sa.sa_handler = startThinking;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_NOCLDSTOP;
        sigaction(SIGUSR1, &sa, NULL);
        do {
            returnValFromWaitPid = waitpid(pid, NULL, 0);
        } while (returnValFromWaitPid == -1);
    }
}