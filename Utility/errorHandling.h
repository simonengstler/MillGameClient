#pragma once
#include "../Config/config.h"
#include "../Connector/client.h"

void handleError(int errorNumber, char *errorMessage);
void checkGameId(char *gameId);
void checkPlayerNumber(char *playerNumber);
void checkIfAllSpecificationDetailsAreProvided(configSpecification *configSpec);
void checkIfAllGameInfosAreProvided(sharedGameInformation *gameInfo);
void checkIfAllPlayerInfosAreProvided(sharedPlayerInformation *playerInfo);
