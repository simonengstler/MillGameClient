#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "errorHandling.h"
#include "../Config/config.h"
#include "../Connector/client.h"

void handleError(int errorNumber, char *errorMessage) {
    errno = errorNumber;
    perror(errorMessage);
    exit(EXIT_FAILURE);
}

void checkGameId(char *gameId) {
    if (strlen(gameId) != 13) {
        handleError(EINVAL, "GameId has to be 13 characters long and the game must exist.");
    }
}

void checkPlayerNumber(char *playerNumber) {
    if (strlen(playerNumber) != 1) {
        handleError(ENOTCONN, "PlayerNumber has to be either 0, 1 or empty.");
    }
}

void checkIfAllSpecificationDetailsAreProvided(configSpecification *configSpec) {
    if(strlen(configSpec->hostName) == 0 || strlen(configSpec->gameKind) == 0 || configSpec->portNumber == -1){
        handleError(EINVAL, "The given config specification file is incomplete");
    }
}

void checkIfAllGameInfosAreProvided(sharedGameInformation *gameInfo) {
    if(gameInfo->numberOfPlayers == -1 || gameInfo->clientPlayerNumber == -1 || gameInfo->connectorPID == -1
        || strlen(gameInfo->gameName) == 0 || gameInfo->thinkerPID == -1) {
        handleError(EINVAL, "The extracted game information is incomplete");
    }
}

void checkIfAllPlayerInfosAreProvided(sharedPlayerInformation *playerInfo) {
    if(playerInfo->playerNumber == -1 || strlen(playerInfo->playerName) == 0) {
        handleError(EINVAL, "The extracted player information is incomplete");
    }
}
