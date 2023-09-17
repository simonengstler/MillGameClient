#pragma once
#include "../Connector/client.h"

void *startProcessCommunication(int shmID);
void readResultOfThinking(int pipeFd, char *resultOfThinking);
