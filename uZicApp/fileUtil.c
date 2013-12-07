#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "musicProtocol.h"
#include "fileUtil.h"
#include <string.h>
#include <openssl/md5.h>

int doDiffServer ( msg_t *rcvInfo, msg_t *sndInfo )
{
    int i = 0;
    DIR *dir;
    struct dirent *ent;
    char tmpStr[128];
    char filepath[FILENAME_MAX ];
    unsigned checksum;

    strcpy ( sndInfo->request, rcvInfo->request );
    if ( ( ( dir = opendir ( "./repo" ) ) != NULL ) )
    {
        while (  ( ent = readdir ( dir ) ) != NULL )
        {
            char *d_name = ent->d_name;

            memset ( filepath, 0, FILENAME_MAX );
            strcat ( filepath, SERVER_DIR );
            strcat ( filepath, d_name );
            checksum = 0;

            if ( *d_name != '.' )
            {
                strcpy ( sndInfo->filenames[i], d_name );

                unsigned char c[MD5_DIGEST_LENGTH];
                int j;

                char temp[3];
                memset( &temp, 0, sizeof(temp) );

                FILE *fp = fopen ( filepath, "rb" );
                MD5_CTX mdContext;
                int bytes;
                unsigned char data[1024];
                
                if(fp == NULL){
                    printf("%s can't be opened.\n", filepath);
                    exit(1);
                }
                MD5_Init(&mdContext);
                while((bytes = fread(data, 1, 1024, fp)) != 0){
                    MD5_Update(&mdContext, data, bytes);
                }
                MD5_Final(c, &mdContext);
                for(j = 0; j < MD5_DIGEST_LENGTH; j++) {
                    checksum += c[j];
                    /*sprintf(temp, "%02x", c[j]);
                    printf("temp hash: %s\n", temp);*/
                }
                fclose(fp);

                sndInfo->cksums[i++] = checksum;

            }
        }
    }

    return i;
}

int doDiffClient ( msg_t *rcvInfo, msg_t *sndInfo )
{
    DIR *dir;
    struct dirent *ent;
    int i;
    int j;
    int numClientFiles = 0;
    int numUnmatched = 0;
    char isFound;
    char fnBuf[128][FILENAME_MAX];
    int cksumBuf[32];
    char tmpStr[128];
    char filepath[FILENAME_MAX];
    struct stat s;
    unsigned checksum = 0;

    /* Get files on client first */
    if ( ( dir = opendir ( UZIC_DIR ) ) != NULL )
    {
        while ( ( ent = readdir ( dir ) ) != NULL )
        {
            char *d_name = ent->d_name;
            memset ( filepath, 0, FILENAME_MAX );
            strcat ( filepath, UZIC_DIR );
            strcat ( filepath, d_name );
            stat ( filepath, &s );
            checksum = 0;

            if ( *d_name != '.' )
            {
                strcpy ( fnBuf[numClientFiles], d_name );

                unsigned char c[MD5_DIGEST_LENGTH];
                int j;

                char temp[3];
                memset( &temp, 0, sizeof(temp) );

                FILE *fp = fopen ( filepath, "rb" );
                MD5_CTX mdContext;
                int bytes;
                unsigned char data[1024];
                
                if(fp == NULL){
                    printf("%s can't be opened.\n", filepath);
                    exit(1);
                }
                MD5_Init(&mdContext);
                while((bytes = fread(data, 1, 1024, fp)) != 0){
                    MD5_Update(&mdContext, data, bytes);
                }
                MD5_Final(c, &mdContext);
                for(j = 0; j < MD5_DIGEST_LENGTH; j++) {
                    checksum += c[j];
                    /*sprintf(temp, "%02x", c[j]);
                    printf("temp hash: %s\n", temp);*/
                }
                fclose(fp);

                cksumBuf[numClientFiles++] = checksum;
            }
        }
    }

    /* Find diff of arrays */
    for ( i = 0; i < rcvInfo->len; i++ )
    {
        isFound = 0;
        for ( j = 0; j < numClientFiles; j++ )
        {
            /* Client filename match found */
            if ( rcvInfo->cksums[i] == cksumBuf[j] )
            {
                isFound = 1;
                break;
            }
        }

        if ( !isFound )
        {
            strcpy ( sndInfo->filenames[numUnmatched++], rcvInfo->filenames[i] );
        }
    }

    return numUnmatched;
}

int doCapServer( msg_t *rcvInfo, msg_t *sndInfo, int capSize ) {
    capInfo *capNodes = getAllowedFiles();

    capInfo *ptr = capNodes;

    int i = 0;
    int size = 0;
    printf("Cap size: %s\n", capSize);
    capSize *= 1000000;
    while ( ptr != NULL )
    {
        printf("Size: %s\n", size);
        if((size + ptr->size) < capSize) {
            strcpy(sndInfo->filenames[i++], ptr->name);
            printf("Current song name: %s\n", ptr->name);
            size += ptr->size;
        }
        ptr = ptr->next;
    }

    freeNodes ( &capNodes );
    return 0;
}



void insertInOrder ( capInfo **capNodes, capInfo **new )
{
    capInfo *cur = *capNodes;
    /* Case head is NULL */
    if ( cur == NULL )
    {
        *capNodes = *new;
        ( *new )->next = NULL;
        return;
    }

    /* Case bigger than HEAD */
    if ( ( *new )->playCount > cur->playCount )
    {
        capInfo *tmp = *capNodes;
        *capNodes = *new;
        ( *new )->next = tmp;
        return;
    }

    /* Case insert in between */
    while ( cur->next != NULL && cur->next->playCount > ( *new )->playCount )
    {
        cur = cur->next;
    }

    capInfo *tmp = cur->next;
    cur->next = *new;
    ( *new )->next = tmp;
}

char *getNextNode ( char **ptr, const char *tag )
{
    int i = 0;
    char *ret = malloc ( FILENAME_MAX );;
    memset ( ret, 0, FILENAME_MAX );
    *ptr = strstr ( *ptr, tag );
    if ( *ptr == NULL )
        return NULL;

    *ptr = strstr ( ++ ( *ptr ), ">" );
    *ptr = strstr ( ++ ( *ptr ), ">" );
    *ptr = strstr ( ++ ( *ptr ), ">" );
    ++ ( *ptr );

    while ( ( *ptr ) [i] != '<' )
    {
        ret[i] = ( *ptr ) [i];
        i++;
    }

    return ret;
}

capInfo *getAllowedFiles()
{
    char *iTunesXML = "iTunes Music Library.xml";
    struct stat s;
    int i = 0;
    char *nameBuf;
    char *sizeBuf;
    char *cntBuf;
    capInfo *ptr;
    capInfo *capNodes = NULL;

    stat ( iTunesXML, &s );
    FILE *fp = fopen ( iTunesXML, "r" );
    char *xmlBuffer = malloc ( s.st_size );

    int totBytesRead = 0, bytesRead = 0;
    while ( totBytesRead < s.st_size )
    {
        bytesRead = fread ( xmlBuffer + totBytesRead, 1, 1024, fp ) ;
        totBytesRead += bytesRead;
    }
    fclose ( fp );

    char *nextSong = xmlBuffer;

    // Looping
    while ( nextSong != NULL )
    {
        nameBuf = getNextNode ( &nextSong, TAG_NAME );
        sizeBuf = getNextNode ( &nextSong, TAG_SIZE );
        cntBuf = getNextNode ( &nextSong, TAG_COUNT );
        if ( cntBuf != NULL )
        {
            capInfo *new = malloc ( sizeof ( capInfo ) );
            strcpy ( new->name, nameBuf );
            new->size = atoi ( sizeBuf );
            new->playCount = atoi ( cntBuf );
            insertInOrder ( &capNodes, &new );
        }

        free ( nameBuf );
        free ( sizeBuf );
        free ( cntBuf );
    }

    free ( xmlBuffer );
    return capNodes;
}

void freeNodes ( capInfo **capNodes )
{
    capInfo *ptr = *capNodes;
    while ( *capNodes != NULL )
    {
        ptr = ( *capNodes )->next;
        free ( *capNodes );
        *capNodes = ptr;
    }

}

