#pragma once
#include "../Connector/client.h"

void emptyBuffer ();

void communicateGameSetup (int socketFd, char *gameId, char *clientPlayerNumber, sharedGameInformation *sharedGameInfo, sharedPlayerInformation *sharedPlayerInfo);
void handleError(int errorNumber, char *errorMessage);
void readFromServerAndPrint(int socketFd);
bool readGameStatus(int socketFd, sharedGameInformation *sharedGameInfo);
void sendToServer(int socketFd, char *specifier, char *message);



