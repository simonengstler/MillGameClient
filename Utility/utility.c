#include "utility.h"
#include <string.h>
#include <stdbool.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include "errorHandling.h"
#include "../Connector/client.h"


bool checkStartsWith(const char *receivedString, const char *stringToMatch) {
    return strncmp(receivedString, stringToMatch, strlen(stringToMatch)) == 0;
}

void removeSpaces(char *s, char*d) {
    int counterD = 0;
    int counterS = 0;
    while ((s[counterS] != '\r') && (s[counterS] != '\n')) {
        if (s[counterS] != ' ') {
            d[counterD] = s[counterS];
            counterD++;
        }
        counterS++;
    }
    d[counterD] = '\0';
}

void cutAtFirstLineBreak(char *s) {
    int counter = 0;
    while (s[counter] != '\n') {
        counter++;
    }
    s[counter] = '\0';
}

void processArgs(int argc, char *argv[], char *gameId, char *playerNumber, char *configFileName) {
    int option;
    bool gameIdArgIsGiven = false;
    bool playerNumberArgIsGiven = false;
    while ((option = getopt(argc, argv, "g:p:c:")) != -1) {
        switch (option) {
            case 'g':
                strcpy(gameId, optarg);
                checkGameId(gameId);
                gameIdArgIsGiven = true;
                break;
            case 'p':
                strcpy(playerNumber, optarg);
                checkPlayerNumber(playerNumber);
                playerNumberArgIsGiven = true;
                break;
            case 'c':
                strcpy(configFileName, optarg);
                break;
            default:
                handleError(EINVAL, "Please check the console parameters");
                break;
        }
    }
    if (!(gameIdArgIsGiven)) {
        handleError(1, "Mandatory argument(s) missing");
    }
    if(!playerNumberArgIsGiven) {
        printf("<<No player number provided. Waiting for player number from server>>\n");
    }
}

void emptyString (char *str) {
    strcpy(str, "");
}

char returnSymbolForPlayingField (int owner) {
    switch (owner) {
        case -1: return '+';
        case 0: return '0';
        case 1: return '1';
        default:
            handleError(EIO, "Playing Field contains unexpected value");
            return 'E';
    }
}

void printLongestLine(sharedGameInformation *sharedGameInfo, int firstField, int secondField, int thirdField) {
    printf("%c–––––––––––", returnSymbolForPlayingField(sharedGameInfo->playingField[0][firstField])); //upper left hand corner
    printf("%c–––––––––––", returnSymbolForPlayingField(sharedGameInfo->playingField[0][secondField])); //upper middle
    printf("%c\n", returnSymbolForPlayingField(sharedGameInfo->playingField[0][thirdField])); //upper right hand corner
}

void printMiddleLine(sharedGameInformation *sharedGameInfo, int firstField, int secondField, int thirdField) {
    printf("|   %c–––––––", returnSymbolForPlayingField(sharedGameInfo->playingField[1][firstField]));
    printf("%c–––––––", returnSymbolForPlayingField(sharedGameInfo->playingField[1][secondField]));
    printf("%c   |\n", returnSymbolForPlayingField(sharedGameInfo->playingField[1][thirdField]));
}

void printShortestLine(sharedGameInformation *sharedGameInfo, int firstField, int secondField, int thirdField) {
    printf("|   |   %c–––", returnSymbolForPlayingField(sharedGameInfo->playingField[2][firstField]));
    printf("%c–––", returnSymbolForPlayingField(sharedGameInfo->playingField[2][secondField]));
    printf("%c   |   |\n", returnSymbolForPlayingField(sharedGameInfo->playingField[2][thirdField]));
}

void printPartedLine(sharedGameInformation *sharedGameInfo) {
    printf("%c–––", returnSymbolForPlayingField(sharedGameInfo->playingField[0][7]));
    printf("%c–––", returnSymbolForPlayingField(sharedGameInfo->playingField[1][7]));
    printf("%c       ", returnSymbolForPlayingField(sharedGameInfo->playingField[2][7]));
    printf("%c–––", returnSymbolForPlayingField(sharedGameInfo->playingField[2][3]));
    printf("%c–––", returnSymbolForPlayingField(sharedGameInfo->playingField[1][3]));
    printf("%c\n", returnSymbolForPlayingField(sharedGameInfo->playingField[0][3]));
}

void printPlayingField(sharedGameInformation *sharedGameInfo) {
    //upper half
    printLongestLine(sharedGameInfo, 0, 1, 2);
    printf("|           |           |\n");
    printMiddleLine(sharedGameInfo, 0, 1, 2);
    printf("|   |       |       |   |\n");
    printShortestLine(sharedGameInfo, 0, 1, 2);
    printf("|   |   |       |   |   |\n");
    printPartedLine(sharedGameInfo);

    //lower half
    printf("|   |   |       |   |   |\n");
    printShortestLine(sharedGameInfo, 6, 5, 4);
    printf("|   |       |       |   |\n");
    printMiddleLine(sharedGameInfo, 6, 5, 4);
    printf("|           |           |\n");
    printLongestLine(sharedGameInfo, 6, 5, 4);
}

char itoc(int i) {
    char intAsChar[2];
    sprintf(intAsChar, "%d", i);
    return intAsChar[0];
}

int mod(int number, int modNumber) {
    if(number>=0) {
        return number % modNumber;
    }
    else {
        while(number < 0) {
            number += modNumber;
        }
    }
    return number;
}