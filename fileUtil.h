#define UZIC_DIR "./my_uZic/"
#define SERVER_DIR "./repo/"
#define TAG_NAME "<key>Name</key><string>"
#define TAG_SIZE "<key>Size"
#define TAG_COUNT "<key>Play Count"

typedef struct capInfo
{
    char name[FILENAME_MAX];
    int size;
    int playCount;
    struct capInfo *next;
} capInfo;

int doDiffServer ( msg_t *rcvInfo, msg_t *sndInfo );
int doDiffClient ( msg_t *rcvInfo, msg_t *sndInfo );
int doCapServer( msg_t *rcvInfo, msg_t *sndInfo, int capSize );
void insertInOrder ( capInfo **capNodes, capInfo **new );
char *getNextNode ( char **ptr, const char *tag );
capInfo *getAllowedFiles();
void freeNodes ( capInfo **capNodes );
