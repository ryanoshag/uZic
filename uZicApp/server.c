#include <sys/stat.h>
#include <pthread.h> /* support for POSIX threads */
#include <time.h> /* support for getting local time in log file */
#include "musicProtocol.h" /* declarations of necessary functions and data types */
#include "fileUtil.h"

#define RCVBUFSIZE 512 /* The receive buffer size */
#define SNDBUFSIZE 2048 /* The send buffer size */
#define FILEBUFSIZE 512 /* The file buffer size */
#define MAXPENDING 5

/* Structure of arguments to pass to client thread */
struct ThreadArgs
{
    int clientSock;	//Socket descriptor for client
};

/* The main function */
int main ( int argc, char *argv[] )
{
    int serverSock;	/* Server Socket */
    int clientSock;	/* Client Socket */
    struct sockaddr_in changeServAddr;	/* Local address */
    struct sockaddr_in changeClntAddr;	/* Client address */
    unsigned int clntLen;	/* Length of address data struct */
    FILE *logPtr;	/* File pointer for server log */
    struct stat s;

    stat ( "./repo", &s );
    if ( !S_ISDIR ( s.st_mode ) )
    {
        printf ( "ERROR: Folder ./repo not detected. Please create it.\n" );
        exit ( 1 );
    }

    /* Create new TCP Socket for incoming requests*/
    if ( ( serverSock = socket ( AF_INET, SOCK_STREAM, IPPROTO_TCP ) ) < 0 )
    {
        exit ( 1 );
    }

    int on = 1;
    setsockopt ( serverSock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof ( on ) );
    memset ( &changeServAddr, 0, sizeof ( changeServAddr ) );
    changeServAddr.sin_family = AF_INET;
    changeServAddr.sin_addr.s_addr = htonl ( INADDR_ANY );
    changeServAddr.sin_port = htons ( 12003 );

    /* Bind to local address structure */
    if ( bind ( serverSock, ( struct sockaddr * ) &changeServAddr, sizeof ( changeServAddr ) ) < 0 )
    {
        perror ( "bind() failed." );
        exit ( 1 );
    }

    /* Listen for incoming connections */
    if ( listen ( serverSock, MAXPENDING ) < 0 )
    {
        perror ( "listen() failed." );
        exit ( 1 );
    }

    /* Loop server forever*/
    while ( 1 )
    {
        /* Accept incoming connection */
        clientSock = accept ( serverSock, ( struct sockaddr * ) &changeClntAddr, &clntLen );
        if ( clientSock < 0 )
        {
            perror ( "accept() failed." );
            exit ( 1 );
        }

        /* Create separate memory for client argument (for threads) */
        struct ThreadArgs *threadArgs = ( struct ThreadArgs * ) malloc ( sizeof ( struct ThreadArgs ) );
        if ( threadArgs == NULL )
        {
            perror ( "malloc() failed" );
            exit ( 1 );
        }
        threadArgs->clientSock = clientSock;

        /* Create client thread */
        pthread_t threadID;
        int returnValue = pthread_create ( &threadID, NULL, ThreadMain, threadArgs );
        if ( returnValue != 0 )
        {
            perror ( "pthread_create() failed" );
            exit ( 1 );
        }

        printf ( "Client connected on socket %d (thread %lu)\n", clientSock, ( unsigned long int ) threadID );

        /* Open server log to write connection information */
        logPtr = fopen ( "serverLog.txt", "a" );

        /* Get current time for server log */
        time_t now = time ( 0 );
        struct tm *localTime = localtime ( &now );

        /* Print message to server log */
        fprintf ( logPtr, "%sClient connected on socket %d (thread %lu)\n\n", asctime ( localTime ), clientSock, ( unsigned long int ) threadID );

        /* Close server log after writing is complete */
        fclose ( logPtr );
    }
}

/**
* ThreadMain
*
* @param threadArgs Void pointer to arguments of thread
**/
void *ThreadMain ( void *threadArgs )
{
    /* Deallocate thread resources on return */
    pthread_detach ( pthread_self() );

    /* Extract socket file descriptor from argument */
    int clientSock = ( ( struct ThreadArgs * ) threadArgs )->clientSock;

    /* Deallocate memory for argument */
    free ( threadArgs );

    /* Call function to handle all requests from client on a thread */
    HandleClientRequest ( clientSock );

    return ( NULL );
}

/**
* HandleClientRequest
*
* @param clientSock Defines the socket on which the request is being handled
**/

void HandleClientRequest ( int clientSock )
{
    char rcvBuf[RCVBUFSIZE];
    char sndBuf[SNDBUFSIZE];
    char filenames[128][FILENAME_MAX];
    msg_t sndInfo;
    msg_t rcvInfo;
    struct stat s;
    int i = 0;
    DIR *dir;
    struct dirent *ent;

    while ( 1 )
    {
        memset ( &sndInfo, 0, sizeof ( sndInfo ) );
        memset ( &rcvInfo, 0, sizeof ( rcvInfo ) );
        i = 0;

        recv ( clientSock, rcvBuf, RCVBUFSIZE, 0 );
        Decode ( rcvBuf, RCVBUFSIZE, &rcvInfo );

        printf("Message decoded, doing request %s\n", rcvInfo->request);
        /* Case LIST */
        if ( strcmp ( rcvInfo.request, LIST )  == 0 )
        {
            strcpy ( sndInfo.request, rcvInfo.request );
            if ( ( ( dir = opendir ( "./repo" ) ) != NULL ) )
            {
                while (  ( ent = readdir ( dir ) ) != NULL )
                {
                    char *d_name = ent->d_name;
                    if ( *d_name != '.' )
                        strcpy ( sndInfo.filenames[i++], d_name );
                }
            }

            sndInfo.len = i;

            Encode ( &sndInfo, sndBuf, SNDBUFSIZE );
            send ( clientSock, sndBuf, SNDBUFSIZE, 0 );
        } 	// end of LIST

        /* Case LEAVE */
        else if ( strcmp ( rcvInfo.request, LEAVE )  == 0 )
        {
            strcpy ( sndInfo.request, rcvInfo.request );
            Encode ( &sndInfo, sndBuf, SNDBUFSIZE );
            send ( clientSock, sndBuf, SNDBUFSIZE, 0 );
            close ( clientSock );
            break;
        }	// end of LEAVE

        /* Case DIFF */
        else if ( strcmp ( rcvInfo.request, DIFF )  == 0 )
        {
            FILE *fp;
            char filepath[FILEBUFSIZE];
            int totBytesRead = 0;
            int bytesRead = 0;

            sndInfo.len = doDiffServer ( &rcvInfo, &sndInfo );

            /* Send server file lengths to client */
            Encode ( &sndInfo, sndBuf, SNDBUFSIZE );
            send ( clientSock, sndBuf, SNDBUFSIZE, 0 );
        }	// end of DIFF

        else if ( strcmp ( rcvInfo.request, PULL )  == 0 )
        {
            FILE *fp;
            char filepath[FILEBUFSIZE];
            int totBytesRead = 0;
            int bytesRead = 0;

            strcpy ( sndInfo.request, rcvInfo.request );

            sndInfo.len = doDiffServer ( &rcvInfo, &sndInfo );

            /* Send server file lengths to client */
            Encode ( &sndInfo, sndBuf, SNDBUFSIZE );
            send ( clientSock, sndBuf, SNDBUFSIZE, 0 );

            /* Receive file list of unmatched files */
            recv ( clientSock, rcvBuf, RCVBUFSIZE, 0 );
            Decode ( rcvBuf, RCVBUFSIZE, &rcvInfo );

            for ( i = 0; i < rcvInfo.len; i++ )
            {
                totBytesRead = 0;
                memset ( filepath, 0, sizeof ( filepath ) );
                strcat ( filepath, SERVER_DIR );
                strcat ( filepath, rcvInfo.filenames[i] );
                stat ( filepath, &s );

                /* Send size */
                memset ( sndBuf, 0, SNDBUFSIZE );
                sprintf ( sndBuf, "%lu", s.st_size );
                send ( clientSock, sndBuf, SNDBUFSIZE, 0 );

                /* Wait for acknowledgement */
                recv ( clientSock, rcvBuf, RCVBUFSIZE, 0 );

                fp = fopen ( filepath, "rb" );
                while ( totBytesRead < s.st_size )
                {
                    memset ( sndBuf, 0, SNDBUFSIZE );
                    bytesRead = fread ( sndBuf, 1, SNDBUFSIZE, fp );
                    totBytesRead += bytesRead;
                    send ( clientSock, sndBuf, bytesRead, 0 );
                }

                fclose ( fp );

                /* Wait for acknowledge current file is done writing */
                recv ( clientSock, rcvBuf, RCVBUFSIZE, 0 );
            }
        }	// end of PULL
        else if ( strcmp ( rcvInfo.request, CAP )  == 0 ) 
        {
            printf("In cap block\n");
            FILE *fp;
            char filepath[FILEBUFSIZE];
            int totBytesRead = 0;
            int bytesRead = 0;

            strcpy ( sndInfo.request, rcvInfo.request );

            sndInfo.len = doCapServer ( &rcvInfo, &sndInfo, rcvInfo.len );
            //sndInfo.len = 3;

            /* Send server file lengths to client */
            Encode ( &sndInfo, sndBuf, SNDBUFSIZE );
            send ( clientSock, sndBuf, SNDBUFSIZE, 0 );

            /* Receive file list of unmatched files */
            recv ( clientSock, rcvBuf, RCVBUFSIZE, 0 );
            Decode ( rcvBuf, RCVBUFSIZE, &rcvInfo );

            for ( i = 0; i < rcvInfo.len; i++ )
            {
                totBytesRead = 0;
                memset ( filepath, 0, sizeof ( filepath ) );
                strcat ( filepath, SERVER_DIR );
                strcat ( filepath, rcvInfo.filenames[i] );
                stat ( filepath, &s );

                /* Send size */
                memset ( sndBuf, 0, SNDBUFSIZE );
                sprintf ( sndBuf, "%lu", s.st_size );
                send ( clientSock, sndBuf, SNDBUFSIZE, 0 );

                /* Wait for acknowledgement */
                recv ( clientSock, rcvBuf, RCVBUFSIZE, 0 );

                fp = fopen ( filepath, "rb" );
                while ( totBytesRead < s.st_size )
                {
                    memset ( sndBuf, 0, SNDBUFSIZE );
                    bytesRead = fread ( sndBuf, 1, SNDBUFSIZE, fp );
                    totBytesRead += bytesRead;
                    send ( clientSock, sndBuf, bytesRead, 0 );
                }

                fclose ( fp );

                /* Wait for acknowledge current file is done writing */
                recv ( clientSock, rcvBuf, RCVBUFSIZE, 0 );
            }
        }   // end of CAP
    }
}

