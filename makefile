CC       = gcc
CFLAGS   = -Wall -Werror -std=c99
OBJFILES = Connector/client.c Utility/errorHandling.c Utility/utility.c Config/config.c Communication/serverCommunication.c Communication/processCommunication.c Thinker/thinker.c
TARGET   = sysprak-client

$(TARGET): $(OBJFILES)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJFILES)

clean:
	rm -f $(TARGET)