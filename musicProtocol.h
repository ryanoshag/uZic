/* Included libraries */

#include <string.h> 		/* support any string ops */
#include <stdint.h>
#include <stdbool.h> 		/* support for booleans */
#include <stdlib.h> 		/* supports all sorts of functionality */
#include <stdio.h> 		/* support for all file I/O functions */
#include <sys/socket.h>		/* for socket(), connect(), send(), and recv() */
#include <arpa/inet.h>		/* for sockaddr_in and inet_addr() */
#include <unistd.h> 		/* for close() */
#include <dirent.h> 		/* support for opening directories and finding file information */

static const char *LIST = "list";
static const char *DIFF = "diff";
static const char *PULL = "pull";
static const char *LEAVE = "leave";
static const char *CAP = "cap";
static const char *DELIM_INFO = "\t";
static const char *DELIM_SONG = "|";

/* Structure for sending and handling music files */
typedef struct msg_t
{
    char request[12];
    char filenames[32][128];
    int cksums[32];
    int len;
} msg_t;

typedef struct file_info_t
{
    char filename[FILENAME_MAX];
    size_t filesize;
} file_info_t;

size_t Encode ( const msg_t *music, uint8_t *outBuf, const size_t bufSize );	/* Encode messages for sending */
bool Decode ( uint8_t *inBuf, const size_t mSize, msg_t *music );	/* Decode sent messages */
void HandleClientRequest ( int clientSock );	/* All server-side work for handling requests from a client */
void *ThreadMain ( void *arg );	/* Main program for a thread */
