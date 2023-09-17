#define _POSIX_SOURCE

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <errno.h>
#include "serverCommunication.h"
#include "../Utility/errorHandling.h"
#include "../Utility/utility.h"
#include "../Connector/client.h"

#define BUF 256
#define CLIENT_VERSION "3.42"

char buffer[BUF];
int serverMessageCounter = 0;
bool nextIsGameName = false;

void readFromServer(int socketFd) {
    emptyString(buffer);
    bool endOfLine = false;
    int counter = 0;
    char nextCharacter;
    int returnValue;
    do {
        returnValue = read(socketFd, &nextCharacter, 1);
        if (returnValue < 0) {
            handleError(ECONNABORTED, "Error when reading from server.");
        }
        buffer[counter] = nextCharacter;
        counter++;
        if (nextCharacter == '\n') {
            endOfLine = true;
            buffer[counter] = '\0';
        }
    } while (!endOfLine);

    if (buffer[0] != '+') {
        handleError(ECONNABORTED, buffer);
    }
}

void readFromServerAndPrint(int socketFd) {
    char modifiedMessage[256];

    readFromServer(socketFd);

    strcpy(modifiedMessage, (buffer + 2));
    printf("Server: %s", modifiedMessage);
}

int getClientPlayerNumber(char *modifiedMessage) {
    char readClientPlayerNumber[2];
    int clientPlayerNumber;
    readClientPlayerNumber[0] = modifiedMessage[4];
    readClientPlayerNumber[1] = '\0';
    clientPlayerNumber = atoi(readClientPlayerNumber);
    return clientPlayerNumber;
}

int getNumberOfPlayers(char *modifiedMessage) {
    char readNumberOfPlayers[2];
    int numberOfPlayers;
    readNumberOfPlayers[0] = modifiedMessage[6];
    readNumberOfPlayers[1] = '\0';
    numberOfPlayers = atoi(readNumberOfPlayers);
    return numberOfPlayers;
}

void processOpponent(char *modifiedMessage, sharedPlayerInformation *sharedPlayerInfo) {
    char readOpponentPlayerNumber[2];
    char readPlayerStatus[2];
    int playerStatus;
    int opponentPlayerNumber;
    char playerName[strlen(modifiedMessage) - 4];

    readOpponentPlayerNumber[0] = modifiedMessage[0];
    readOpponentPlayerNumber[1] = '\0';
    readPlayerStatus[0] = modifiedMessage[strlen(modifiedMessage) - 2];
    readPlayerStatus[1] = '\0';
    strncpy(playerName, (modifiedMessage + 2), strlen(modifiedMessage) - 5);
    playerName[strlen(modifiedMessage) - 5] = '\0';

    playerStatus = atoi(readPlayerStatus);
    opponentPlayerNumber = atoi(readOpponentPlayerNumber);

    strcpy(sharedPlayerInfo->playerName, playerName);
    sharedPlayerInfo->playerIsReady = playerStatus;
    sharedPlayerInfo->playerNumber = opponentPlayerNumber;

    if (playerStatus == 1) {
        printf("Server: Player %s with player number %d is ready\n", playerName, opponentPlayerNumber);
    } else {
        printf("Server: Player %s with player number %d is not ready\n", playerName, opponentPlayerNumber);
    }
}


//TODO: eventuell error-handling: startsWith prüfen
void processServerCommunicationProlog(sharedGameInformation *sharedGameInfo,
                                      sharedPlayerInformation *sharedPlayerInfo) {
    char modifiedMessage[256];
    int clientPlayerNumber;
    int numberOfPlayers;
    strcpy(modifiedMessage, (buffer + 2));

    switch (serverMessageCounter) {
        case 0: //Accepting Connections
            printf("Server: %s", modifiedMessage);
            break;
        case 2: //Please send game-id
            printf("Server: %s", modifiedMessage);
            break;
        case 3: //Playing ...
            printf("Server: %s", modifiedMessage);
            break;
        case 4: //GameName
            printf("Server: Game name is %s", modifiedMessage);
            cutAtFirstLineBreak(modifiedMessage);
            strcpy(sharedGameInfo->gameName, modifiedMessage);
            break;
        case 5: //clientPlayerNumber
            clientPlayerNumber = getClientPlayerNumber(modifiedMessage);
            printf("Server: Your player number is %d\n", clientPlayerNumber);
            sharedGameInfo->clientPlayerNumber = clientPlayerNumber;
            break;
        case 6: //TotalNumberOfPlayers
            numberOfPlayers = getNumberOfPlayers(modifiedMessage);
            printf("Server: The total number of Players is %d\n", numberOfPlayers);
            sharedGameInfo->numberOfPlayers = numberOfPlayers;
            break;
        case 7: //PlayerStatus
            processOpponent(modifiedMessage, sharedPlayerInfo);
            break;
        default:
            printf("Server: %s", modifiedMessage);
    }
    serverMessageCounter++;

}

void prepareMessage(char *specifier, char *message) {
    emptyString(buffer);
    if (checkStartsWith(specifier, "OKWAIT")) {
        strcpy(buffer, specifier);
        strcat(buffer, "\n");
    } else if (checkStartsWith(specifier, "PLAY") && (!checkStartsWith(specifier, "PLAYER"))) {
        strcpy(buffer, specifier);
        strcat(buffer, " ");
        strcat(buffer, message);
    } else if (checkStartsWith(specifier, "PLAY") && (strlen(message) == 0)) {
        strcpy(buffer, specifier);
        strcat(buffer, "\n");
    }
    else {
        strcpy(buffer, specifier);
        strcat(buffer, " ");
        strcat(buffer, message);
        strcat(buffer, "\n");
    }
}

void sendToServer(int socketFd, char *specifier, char *message) {
    int returnValue;
    prepareMessage(specifier, message);
    returnValue = send(socketFd, buffer, strlen(buffer), 0);
    if (returnValue < 0) {
        handleError(ECONNABORTED, "Error when sending to server.");
    }
    printf("Client: %s", buffer);
}

void readFromServerNumberOfTimesProlog(int socketFd, int numberOfReads, sharedGameInformation *sharedGameInfo,
                                       sharedPlayerInformation *sharedPlayerInfo) {
    for (int count = 1; count <= numberOfReads; count++) {
        readFromServer(socketFd);
        processServerCommunicationProlog(sharedGameInfo, sharedPlayerInfo);
    }
}

void resetPlayingField(sharedGameInformation *sharedGameInfo) {
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 8; ++j) {
            sharedGameInfo->playingField[i][j]=-1;
        }
    }
}

void processPlayingField(int socketFd, sharedGameInformation *sharedGameInfo) {
    resetPlayingField(sharedGameInfo);
    for (int i = 0; i < 18; i++) {
        int row = -1;
        int posInRow = -1;
        int owner = -1;
        char charPosInRow[2];
        char charOwner[2];

        readFromServer(socketFd);

        if (strlen(buffer) == 14) {
            if (buffer[11] == 'A') row = 0;
            else if (buffer[11] == 'B') row = 1;
            else if (buffer[11] == 'C') row = 2;

            charPosInRow[0] = buffer[12];
            charOwner[0] = buffer [7];
            charPosInRow[1] = '\0';
            charOwner[1] = '\0';
            posInRow = atoi(charPosInRow);
            owner = atoi(charOwner);
            sharedGameInfo->playingField[row][posInRow] = owner;
        }
    }
}

void processServerMsgMove(int socketFd, sharedGameInformation *sharedGameInfo) {
    char charNumberOfStonesToCapt[2];
    char readMaxThinkingTime[strlen(buffer) - strlen("+ MOVE\n")];
    strncpy(readMaxThinkingTime, (buffer + strlen("+ MOVE ")), strlen(buffer) - strlen("+ MOVE\n"));
    readMaxThinkingTime[strlen(buffer) - strlen("+ MOVE\n") + 1] = '\0';

    readFromServerAndPrint(socketFd); // CAPTURE
    charNumberOfStonesToCapt[0] = buffer[10];
    charNumberOfStonesToCapt[1] = '\0';
    sharedGameInfo->numberOfStonesToCapture = atoi(charNumberOfStonesToCapt);

    readFromServer(socketFd); // PIECELIST

    processPlayingField(socketFd, sharedGameInfo);

    readFromServer(socketFd); // ENDPIECELIST

    sendToServer(socketFd, "THINKING", "");
    readFromServerAndPrint(socketFd);

    sharedGameInfo->thinkingPermitted = true;
    kill(getppid(), SIGUSR1);
}

void processServerMsgWait(int socketFd) {
    sendToServer(socketFd, "OKWAIT", "");
}

void processServerMsgGameover(int socketFd, sharedGameInformation *sharedGameInfo) {

    readFromServer(socketFd); // PIECELIST
    processPlayingField(socketFd, sharedGameInfo);
    readFromServer(socketFd); // ENDPIECELIST
    readFromServerAndPrint(socketFd); // PLAYER0WON
    readFromServerAndPrint(socketFd); // PLAYER1WON
    readFromServerAndPrint(socketFd); // QUIT

}

bool processServerMsgInGame(int socketFd, sharedGameInformation *sharedGameInfo) {
    bool gameIsOver = false;

    if (checkStartsWith(buffer, "+ WAIT")) {
        processServerMsgWait(socketFd);
    } else if (checkStartsWith(buffer, "+ MOVEOK")) {
        //do nothing
    } else if (checkStartsWith(buffer, "+ MOVE")) {
        processServerMsgMove(socketFd, sharedGameInfo);
    } else if (checkStartsWith(buffer, "+ GAMEOVER")) {
        processServerMsgGameover(socketFd, sharedGameInfo);
        gameIsOver = true;
    } else if (checkStartsWith(buffer, "+ QUIT")) {
        handleError(EPROTO, "Protocol Error");
    } else {
        handleError(EBADMSG, "Cannot handle recieved messagecode");
    }

    return gameIsOver;

}


int getNumberOfReads() {
    switch (serverMessageCounter) {
        case 0:
            return 1;
        case 1:
            return 2;
        case 3:
            return 2;
        case 5:
            return 4;
        default:
            printf("Attention: unintended use of 'getNumberOfReads'\n");
            return 1;
    }
}

void readNextMessagesFromServer(int socketFd, sharedGameInformation *sharedGameInfo,
                                sharedPlayerInformation *sharedPlayerInfo) {
    int numberOfReads = getNumberOfReads();
    readFromServerNumberOfTimesProlog(socketFd, numberOfReads, sharedGameInfo, sharedPlayerInfo);
}

void communicateGameSetup(int socketFd, char *gameId, char *clientPlayerNumber, sharedGameInformation *sharedGameInfo,
                          sharedPlayerInformation *sharedPlayerInfo) {
    readNextMessagesFromServer(socketFd, sharedGameInfo, sharedPlayerInfo);
    sendToServer(socketFd, "VERSION", CLIENT_VERSION);
    readNextMessagesFromServer(socketFd, sharedGameInfo, sharedPlayerInfo);
    sendToServer(socketFd, "ID", gameId);
    readNextMessagesFromServer(socketFd, sharedGameInfo, sharedPlayerInfo);
    if(strlen(clientPlayerNumber) == 0) {
        sendToServer(socketFd, "PLAYER", "");
    }
    else{
        sendToServer(socketFd, "PLAYER", clientPlayerNumber);
    }
    readNextMessagesFromServer(socketFd, sharedGameInfo, sharedPlayerInfo);
}

bool readGameStatus(int socketFd, sharedGameInformation *sharedGameInfo) {
    readFromServerAndPrint(socketFd);
    return processServerMsgInGame(socketFd, sharedGameInfo);
}

