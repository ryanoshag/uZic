#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include "musicProtocol.h"	/* declarations of necessary functions and data types */
#include "fileUtil.h"

/* Constants */
#define RCVBUFSIZE 2048		    /* The receive buffer size */
#define SNDBUFSIZE 512		    /* The send buffer size */
#define TMPBUFSIZE 512		    /* Size for temporary buffers */
#define INPUTSIZE 12

//char *servIP = "130.207.114.21";
//char *servIP = "localhost";

char *servIP = "192.168.1.68";
/* The main function */
int main ( int argc, char *argv[] )
{
    struct sockaddr_in serv_addr;   /* The server address */
    FILE *fp = NULL;
    char sndBuf[SNDBUFSIZE];
    char rcvBuf[RCVBUFSIZE];
    char input[INPUTSIZE];
    msg_t sndInfo;
    msg_t rcvInfo;
    int clientSock;		    /* socket descriptor */
    int i, j;

    memset ( sndBuf, 0, SNDBUFSIZE );
    memset ( rcvBuf, 0, RCVBUFSIZE );
    memset ( &sndInfo, 0, sizeof ( sndInfo ) );
    memset ( &rcvInfo, 0, sizeof ( rcvInfo ) );

    /* Create a new TCP socket*/
    clientSock = socket ( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    if ( clientSock < 0 )
        fprintf ( stderr, "Fail to initialize socket\n" );
    memset ( &serv_addr, 0, sizeof ( serv_addr ) );
    serv_addr.sin_family = AF_INET;
    int ret = inet_pton ( AF_INET, servIP, &serv_addr.sin_addr.s_addr );
    serv_addr.sin_port = htons ( 12003 );

    /* Establish connecction to the server */
    if ( ( ret = connect ( clientSock, ( struct sockaddr * ) &serv_addr, sizeof ( serv_addr ) ) ) < 0 )
    {
        fprintf ( stderr, "connection failed\n" );
        exit ( 1 );
    }

    /* Do input loop */
    printf ( "Welcome to uZic! Type list, diff, pull to proceed.\n" );
    while ( 1 )
    {
        memset ( input, 0, INPUTSIZE );
        /* Get input from user */
        printf ( "\n>> " );
        gets ( input );

        /* Check valid input string */
        if ( strcmp ( input, LEAVE ) != 0 && strcmp ( input, LIST ) != 0 && strcmp ( input, DIFF ) != 0 && strcmp ( input, PULL ) != 0   )
        {
            printf ( "ERROR: Command not recognized.\n" );
        }

        else
        {
            /* Send command to server */
            strcpy ( sndInfo.request, input );
            Encode ( &sndInfo, sndBuf, SNDBUFSIZE );
            send ( clientSock, sndBuf, SNDBUFSIZE, 0 );

            /* Get server response */
            size_t bytesRcv = 0;
            while ( bytesRcv < RCVBUFSIZE )
                bytesRcv += recv ( clientSock, rcvBuf + bytesRcv, RCVBUFSIZE, 0 );
            Decode ( rcvBuf, RCVBUFSIZE, &rcvInfo );

            /* Case list */
            if ( strcmp ( input, "list" ) == 0 )
            {
                printf ( "	Songs in server library:\n" );
                for ( i = 0; i < rcvInfo.len; i++ )
                {
                    printf ( "		%s\n", rcvInfo.filenames[i] );
                }
            }	// end of LIST

            /* Case DIFF */
            else if ( strcmp ( input, DIFF ) == 0 )
            {
                char filepath[FILENAME_MAX];
                int numClientFiles = 0;
                int totBytesRcv;
                int bytesRcv;
                int curFileSize;
                int isFound;
                int numUnmatched;

                numUnmatched = doDiffClient ( &rcvInfo, &sndInfo );
                sndInfo.len = numUnmatched;

                if ( numUnmatched == 0 )
                    printf ( "    No files need pulling.\n" );

                for ( i = 0; i < numUnmatched; i++ )
                {
                    printf ( "    %s\n", rcvInfo.filenames[i] );
                }

            }	// end of DIFF

            /* Case LEAVE */
            else if ( strcmp ( input, LEAVE ) == 0 )
            {
                close ( clientSock );
                printf ( "    Bye.\n" );
                exit ( 0 );
            }	// end of LEAVE

            else if ( strcmp ( input, PULL ) == 0 )
            {
                char filepath[FILENAME_MAX];
                int numClientFiles = 0;
                int totBytesRcv;
                int bytesRcv;
                int numUnmatched;
                int curFileSize;
                int isFound;

                numUnmatched = doDiffClient ( &rcvInfo, &sndInfo );
                sndInfo.len = numUnmatched;

                /* Send file list to server */
                Encode ( &sndInfo, sndBuf, SNDBUFSIZE );
                send ( clientSock, sndBuf, SNDBUFSIZE, 0 );

                if ( numUnmatched == 0 )
                    printf ( "    No files need pulling.\n" );

                for ( i = 0; i < numUnmatched; i++ )
                {
                    /* Get length of current file */
                    memset ( rcvBuf, 0, RCVBUFSIZE );
                    recv ( clientSock, rcvBuf, RCVBUFSIZE, 0 );
                    curFileSize = atoi ( rcvBuf );

                    /* Acknowledge filesize received */
                    send ( clientSock, sndBuf, SNDBUFSIZE, 0 );

                    /* Set filepath = DIR + name */
                    memset ( filepath, 0, FILENAME_MAX );
                    strcat ( filepath, UZIC_DIR );
                    strcat ( filepath, sndInfo.filenames[ i ] );

                    printf ( "    Syncing file %s...\n", filepath );
                    fp = fopen ( filepath, "wb" );
                    totBytesRcv = 0;
                    while ( totBytesRcv < curFileSize )
                    {
                        memset ( rcvBuf, 0, RCVBUFSIZE );
                        bytesRcv = recv ( clientSock, rcvBuf, RCVBUFSIZE, 0 );
                        totBytesRcv += bytesRcv;
                        fwrite ( rcvBuf, 1, bytesRcv, fp );
                    }

                    fclose ( fp );
                    send ( clientSock, sndBuf, SNDBUFSIZE, 0 );
                }

                printf ( "    PULL complete.\n" );
            }	// end of PULL

        }	// end of WHILE

    }
    close ( clientSock );
    return 0;
}

