#pragma once

#include <stdbool.h>
#include <netdb.h>


typedef struct {
    char gameName[256];
    int clientPlayerNumber;
    int numberOfPlayers;
    int playingField[3][8];
    pid_t connectorPID;
    pid_t thinkerPID;
    bool thinkingPermitted;
    bool thinkingFinished;
    int numberOfStonesToCapture;
} sharedGameInformation;

typedef struct {
    int playerNumber;
    char playerName[256];
    bool playerIsReady;
} sharedPlayerInformation;

