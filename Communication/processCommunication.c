#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/shm.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <stdbool.h>
#include "../Connector/client.h"
#include "../Utility/errorHandling.h"
#include "../Utility/utility.h"
#include "serverCommunication.h"
#include "processCommunication.h"

void initializeSharedMemoryInformation(sharedGameInformation *sharedGameInfo) {
    memset(sharedGameInfo->gameName, 0, 256);
    sharedGameInfo->clientPlayerNumber = -1;
    sharedGameInfo->numberOfPlayers = -1;
    sharedGameInfo->connectorPID = -1;
    sharedGameInfo->thinkerPID = -1;
    sharedGameInfo->thinkingPermitted = false;
}

void copySharedPlayerInformation(sharedPlayerInformation *sharedPlayerInfoDest, sharedPlayerInformation *sharedPlayerInfoSrc) {
    strcpy(sharedPlayerInfoDest->playerName, sharedPlayerInfoSrc->playerName);
    sharedPlayerInfoDest->playerNumber = sharedPlayerInfoSrc->playerNumber;
    sharedPlayerInfoDest->playerIsReady = sharedPlayerInfoSrc->playerIsReady;
}

void *startProcessCommunication(int shmID) {
    void *sharedMemory;
    if((sharedMemory = shmat(shmID, NULL, 0))==(void *)-1) {
        handleError(EINVAL,"Failed to attach SHM in Connector");
    }
    return sharedMemory;
}

int readFromPipe(int pipeFd, char *messageFromPipe) {
    bool endOfLine = false;
    int counter = 0;
    char nextCharacter;
    int returnValue;
    do {
        returnValue = read(pipeFd, &nextCharacter, 1);
        if (returnValue < 0) {
            handleError(ECONNABORTED, "Error when reading from server.");
        }
        messageFromPipe[counter] = nextCharacter;
        counter++;
        if (nextCharacter == '\n') {
            endOfLine = true;
            messageFromPipe[counter] = '\0';
        }
    } while (!endOfLine && counter < 8);

    if (strlen(messageFromPipe) > 6) {
        handleError(EMSGSIZE, messageFromPipe);
    }

    return counter;
}

void readResultOfThinking(int pipeFd, char *resultOfThinking) {
    int resultLength;
    emptyString(resultOfThinking);
    resultLength = readFromPipe(pipeFd, resultOfThinking);
    if (strlen(resultOfThinking) != resultLength) {
        handleError(EPIPE, "Failed to read result of thinking from pipe.");
    }

}

