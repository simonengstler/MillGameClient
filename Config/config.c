#define _POSIX_C_SOURCE 200809L

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>
#include <getopt.h>

#include "config.h"
#include "../Utility/errorHandling.h"
#include "../Utility/utility.h"

#define REQUIRED_NUMBER_OF_CONFIG_DETAILS 3

char configSpecificationString[256];
int numberOfValidLinesInConfigFile = 0;

void printConfigSpecification(configSpecification *structToPrint) {
    printf("Host name: %s\n", structToPrint->hostName);
    printf("Port number: %d\n", structToPrint->portNumber);
    printf("Game kind: %s\n", structToPrint->gameKind);
}

FILE *openFile(char *configFileName, const char *restrict mode) {
    FILE *datei = NULL;
    if ((datei = fopen(configFileName, mode)) == NULL) {
        handleError(EBADF, "Error with opening the config file.");
    }
    return datei;
}

int getNumberOfLines(FILE *fp) {
    int lines = 0;
    char nextCharacter;
    while (!feof(fp)) {
        nextCharacter = fgetc(fp);
        if (nextCharacter == '\n') {
            lines++;
        }
    }
    return lines;
}

void readLineFromFile(FILE *configFile, char *configSpecificationLine) {
    fgets(configSpecificationLine, 256, configFile);
}

void storeInStruct(configSpecification *configSpec, char *configSpecificationLine) {
    char configDetail[256];
    char withoutSpaces[256];
    removeSpaces(configSpecificationLine, withoutSpaces);
    if (checkStartsWith(withoutSpaces, "HOSTNAME")) {
        strcpy(configDetail, (withoutSpaces + strlen("HOSTNAME=")));
        strcpy(configSpec->hostName, configDetail);
        numberOfValidLinesInConfigFile++;
    } else if (checkStartsWith(withoutSpaces, "PORTNUMBER")) {
        strcpy(configDetail, (withoutSpaces + strlen("PORTNUMBER=")));
        configSpec->portNumber = atoi(configDetail);
        numberOfValidLinesInConfigFile++;
    } else if (checkStartsWith(withoutSpaces, "GAMEKIND")) {
        strcpy(configDetail, (withoutSpaces + strlen("GAMEKIND=")));
        strcpy(configSpec->gameKind, configDetail);
        numberOfValidLinesInConfigFile++;
    }
}

void readConfigFile(char *configFileName, configSpecification *configSpec) {
    FILE *configFile = openFile(configFileName, "r");
    int numberOfLines = getNumberOfLines(configFile);
    rewind(configFile);

    for (int i = 0; i < numberOfLines; i++) {
        char configSpecificationLine[256];
        readLineFromFile(configFile, configSpecificationLine);
        storeInStruct(configSpec, configSpecificationLine);
    }
    checkIfAllSpecificationDetailsAreProvided(configSpec);
    fclose(configFile);
}

void initializeConfigSpecification(configSpecification *configSpecs, char *configFileName) {
    configSpecs->portNumber = -1;
    memset(configSpecs->hostName, 0, 256);
    memset(configSpecs->gameKind, 0, 256);
    if (strlen(configFileName) == 0) {
        readConfigFile("client.conf", configSpecs);
    } else {
        readConfigFile(configFileName, configSpecs);
    }
}