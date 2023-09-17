#include <stdio.h>
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
#include "../Connector/client.h"
#include "../Communication/processCommunication.h"
#include "../Utility/utility.h"
#include "../Utility/errorHandling.h"
#include "thinker.h"

sharedGameInformation *sharedGameInfo;
sharedPlayerInformation *sharedPlayerInfo;
int piecesSet = 0;
char nextMove[7];


void connectStructPointerToSharedMemory(void *sharedMemory) {
    sharedGameInfo = (sharedGameInformation *) sharedMemory;
    sharedPlayerInfo = (sharedPlayerInformation *) (sharedMemory + sizeof(sharedGameInformation));
}

void writeNextTurnInPipe(int pipeFd[2]) {
    if ((write(pipeFd[1], nextMove, strlen(nextMove))) != strlen(nextMove)) {
        handleError(EPIPE, "Failed to write result of thinking into pipe.");
    }
}


char extractRing(int row) {
    switch (row) {
        case 0:
            return 'A';
        case 1:
            return 'B';
        case 2:
            return 'C';
        default:
            handleError(EIO, "Playing Field contains unexpected value");
            return 'E';
    }
}

void checkForMill(int owner) { //PlayerNumber to see own mill, 1-PlayerNumber for opponents mill
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 4; j++) {
            if (sharedGameInfo->playingField[i][(0+2*j)%8] + sharedGameInfo->playingField[i][(1+2*j)%8] + sharedGameInfo->playingField[i][(2+2*j)%8] == 3 * owner) // mills in rings
            {
                // return mills
            }
            if (i == 0 && sharedGameInfo->playingField[0][(1+2*j)%8] + sharedGameInfo->playingField[1][(1+2*j)%8] + sharedGameInfo->playingField[2][(1+2*j)%8] == 3 * owner) // vertical
            // has to be only tested once
            {
                // return mills
            }
        }
    }
}

void setNextPut(int ring, int field) {
    nextMove[0] = extractRing(ring);
    nextMove[1] = itoc(field);
    nextMove[2] = '\n';
    nextMove[3] = '\0';
}

void thinkAboutPutting() {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 8; j++) {
            if (sharedGameInfo->playingField[i][j] == -1) {
                setNextPut(i, j);
                return;
            }
        }
    }
}

void setNextMove(int ringOld, int fieldOld, int ringNew, int fieldNew) {
    nextMove[0] = extractRing(ringOld);
    nextMove[1] = itoc(fieldOld);
    nextMove[2] = ':';
    nextMove[3] = extractRing(ringNew);
    nextMove[4] = itoc(fieldNew);
    nextMove[5] = '\n';
    nextMove[6] = '\0';

}

bool checkWherePieceCanBeMoved(int ring, int field) {
    if (sharedGameInfo->playingField[ring][(field + 1) % 8] == -1) { //if -1: field is free
        setNextMove(ring, field, ring, (field + 1) % 8);
        return true;
    } else if (sharedGameInfo->playingField[ring][mod((field - 1), 8)] == -1) { //if -1: field is free
        setNextMove(ring, field, ring, mod((field - 1), 8));
        return true;
    } else if (field % 2 == 1) {
        switch (ring) {
            case 0:
                if (sharedGameInfo->playingField[ring + 1][field] == -1) {
                    setNextMove(ring, field, ring + 1, field);
                    return true;
                }
                break;
            case 1:
                if (sharedGameInfo->playingField[ring + 1][field] == -1) {
                    setNextMove(ring, field, ring + 1, field);
                    return true;
                } else if (sharedGameInfo->playingField[ring - 1][field] == -1) {
                    setNextMove(ring, field, ring - 1, field);
                    return true;
                }
                break;
            case 2:
                if (sharedGameInfo->playingField[ring - 1][field] == -1) {
                    setNextMove(ring, field, ring - 1, field);
                    return true;
                }
                break;
            default:
                break;
        }
    }
    return false;
}

void thinkAboutMoving() {
    //char ring;
    //char field[2];
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 8; j++) {
            if (sharedGameInfo->playingField[i][j] == sharedGameInfo->clientPlayerNumber) {
                if (checkWherePieceCanBeMoved(i, j)) return;
            }
        }
    }

}

void thinkAboutCapturing() {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 8; j++) {
            if (sharedGameInfo->playingField[i][j] == 1-sharedGameInfo->clientPlayerNumber) {
                setNextPut(i, j);
                return;
            }
        }
    }
}


void think(int pipeFd[2]) {
    emptyString(nextMove); //TODO korrigieren!
    if (sharedGameInfo->numberOfStonesToCapture > 0) {
        thinkAboutCapturing();
        sharedGameInfo->numberOfStonesToCapture--;
    } else if (piecesSet<9) {
        thinkAboutPutting();
        piecesSet++;
    } else {
        thinkAboutMoving();
    }
    writeNextTurnInPipe(pipeFd);
}

void initiateThinking(void *sharedMemory, int pipeFd[2]) {
    connectStructPointerToSharedMemory(sharedMemory);
    printPlayingField(sharedGameInfo);

    if (sharedGameInfo->thinkingPermitted) {
        think(pipeFd);
    }
    sharedGameInfo->thinkingFinished = true;
    sharedGameInfo->thinkingPermitted = false;
}
