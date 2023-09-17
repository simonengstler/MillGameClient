#pragma once
#include <stdbool.h>
#include <stdio.h>
#include "../Connector/client.h"

bool checkStartsWith(const char *receivedString, const char *stringToMatch);
void removeSpaces(char *s, char*d);
void cutAtFirstLineBreak(char *s);
void processArgs(int argc, char *argv[], char *gameId, char *playerNumber, char *configFileName);
void emptyString (char *str);
void printPlayingField(sharedGameInformation *sharedGameInfo);
char itoc(int i);
int mod(int number, int modNumber);