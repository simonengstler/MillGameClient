#pragma once

typedef struct configSpecification {
    char hostName[256];
    int portNumber;
    char gameKind[256];
} configSpecification;

void initializeConfigSpecification(configSpecification *configSpecs, char *configFileName);