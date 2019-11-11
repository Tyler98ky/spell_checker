#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <assert.h>

#include "utility.h"

#define MAX_ARR_SIZE 2048
#define MAX_DICTIONARY_WORD_SIZE 100
#define MAX_DICTIONARY_WORD_COUNT 110000  // 110,000
#define DEFAULT_PORT 12313
#define DEFAULT_DICTIONARY "words"
#define NUM_WORKERS 20
#define MAX_QUEUE_SIZE 2048


FILE* DictionaryUsed;
int PortUsed;
char* WordBank[MAX_DICTIONARY_WORD_COUNT];
int listeningSocket;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t fill = PTHREAD_COND_INITIALIZER;

void initialSetup(int argc, char *const *argv);
int isValidWord(const char* word);
void setupWordBank(void);
void printDictionary();
int open_listenfd(int port);

int isFull();
void enQueue(int element);
int isEmpty();
int deQueue();
void display();
char *removeUnwantedCharacters(const char* word);
void Pthread_mutex_lock(pthread_mutex_t *mutex);
void Pthread_mutex_unlock(pthread_mutex_t *mutex);
void Pthread_cond_signal(pthread_cond_t* cv);
void Pthread_cond_wait(pthread_cond_t* cv, pthread_mutex_t* mutex);

void producer(void);
void *consumer(void*);
void serviceClient(int socketDescriptor);

int main(int argc, char *argv[]) {
    initialSetup(argc, argv);
    setupWordBank();

    listeningSocket = open_listenfd(PortUsed);
    printf("Listening on port %d\n", PortUsed);

    pthread_t threadPool[NUM_WORKERS];
    int threadIDs[NUM_WORKERS];
    printf("Launching threads.\n");
    for (int i = 0; i < NUM_WORKERS; i++) {
        threadIDs[i] = i;
        pthread_create(&threadPool[i], NULL, &consumer, &threadIDs[i]);
    }
    printf("Threads launched. Now listening for incoming requests...\n");

    producer();

    return EXIT_SUCCESS;
}

void producer() {
    int connectionSocket;

    while (1) {
        connectionSocket = accept(listeningSocket, NULL, NULL);  // connectionSocket is the file descriptor
        Pthread_mutex_lock(&lock);
//        char* clientMessage = "Hello! I hope you can see this.\n";
//        send(connectionSocket, clientMessage, strlen(clientMessage), 0);


        while (isFull()) {
            Pthread_cond_wait(&empty, &lock);
        }
        enQueue(connectionSocket);  // "put()"
        Pthread_cond_signal(&fill);
        Pthread_mutex_unlock(&lock);

    }
}

void *consumer(void *arg) {
    while (1) {
        Pthread_mutex_lock(&lock);

        while (isEmpty()) {
            Pthread_cond_wait(&fill, &lock);
        }
        int socketDescriptor = deQueue();  // "get()"
        Pthread_cond_signal(&empty);
        Pthread_mutex_unlock(&lock);

        serviceClient(socketDescriptor);

        // TODO close socket
//        shutdown(socketDescriptor, SHUT_RDWR);
    }
}


void serviceClient(int socketDescriptor) {
    size_t bufferSize = MAX_DICTIONARY_WORD_SIZE * sizeof(char);
    char *buffer = malloc(bufferSize);
    buffer[0] = '\0';

    while (recv(socketDescriptor, buffer, bufferSize, 0) != 0) {
        printf("received: %s\n", buffer);
        char *temp = malloc(MAX_DICTIONARY_WORD_SIZE);
        strcat(temp, removeUnwantedCharacters(buffer));

        if (isValidWord(temp)) {
            strcat(temp, " OK\n");
        } else {
            strcat(temp, " MISSPELLED\n");
        }

        send(socketDescriptor, temp, strlen(temp), 0);
        // TODO write to log
        free(temp);
    }
    free(buffer);
}

void printDictionary() {  // For testing
    int size = sizeof(WordBank) / sizeof(WordBank[0]);
    for (int i = 0; i < size; i++) {
        if (WordBank[i] == NULL) break;
        puts(WordBank[i]);
    }
}

void initialSetup(int argc, char *const *argv) {
    if (argc <= 1) {  // Use default values
        DictionaryUsed = fopen(DEFAULT_DICTIONARY, "r");
        PortUsed = DEFAULT_PORT;
    } else {
        char* dictionaryPath = argv[1];
        char* portNumber = argv[2];
        DictionaryUsed = fopen(dictionaryPath, "r");
        PortUsed = atoi(portNumber);
    }

}

void setupWordBank(void) {
    int i = 0;
    char* buffer = malloc(MAX_DICTIONARY_WORD_SIZE * sizeof(char));
    while (fgets(buffer, MAX_DICTIONARY_WORD_SIZE, DictionaryUsed)) {

        WordBank[i] = removeUnwantedCharacters(buffer);
        i++;

    }

    free(buffer);
}

int isValidWord(const char* word) {
    size_t wordBankSize = sizeof(WordBank) / sizeof(WordBank[0]);

    for (size_t i = 0; i < wordBankSize; i++) {
        if (WordBank[i] == NULL)
            break;  // End of dictionary


        if (!strcasecmp(WordBank[i], removeUnwantedCharacters(word))) {  // including newline
            return 1;
        }



//        char *temp = strdup(word);
//        temp[strlen(temp)-1] = '\000';
//        if (!strcmp(WordBank[i], temp)) {  // without newline
//            return 1;
//        }
    }
    return 0;
}

char *removeUnwantedCharacters(const char* word) {
    char* token = strtok(strdup(word), "\n");
    char* token2 = strtok(token, "\r");

    return token2;
}

int open_listenfd(int port)
{
    int listenfd, optval=1;
    struct sockaddr_in serveraddr;

    /* Create a socket descriptor */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
//    if ((listenfd = socket(AF_LOCAL, SOCK_STREAM, 0)) < 0){
        return -1;
    }

    /* Eliminates "Address already in use" error from bind */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
                   (const void *)&optval , sizeof(int)) < 0){
        return -1;
    }

    //Reset the serveraddr struct, setting all of it's bytes to zero.
    //Some properties are then set for the struct, you don't
    //need to worry about these.
    //bind() is then called, associating the port number with the
    //socket descriptor.
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
//    serveraddr.sin_family = AF_LOCAL;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)port);
    if (bind(listenfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0){
        return -1;
    }

    //Prepare the socket to allow accept() calls. The value 20 is
    //the backlog, this is the maximum number of connections that will be placed
    //on queue until accept() is called again.
    if (listen(listenfd, 20) < 0){
        return -1;
    }

    return listenfd;
}

int items[MAX_QUEUE_SIZE];  // the actual queue
volatile int front = -1, rear =-1;
int isFull()
{
    if( (front == rear + 1) || (front == 0 && rear == MAX_QUEUE_SIZE-1)) return 1;
    return 0;
}
int isEmpty()
{
    if(front == -1) return 1;
    return 0;
}
void enQueue(int element)
{
    if (isFull()) printf("\n Queue is full!! \n");
    else
    {
        if(front == -1) front = 0;
        rear = (rear + 1) % MAX_QUEUE_SIZE;
        items[rear] = element;
        printf("\n Inserted -> %d", element);
    }
}
int deQueue()
{
    int element;
    if(isEmpty()) {
        printf("\n Queue is empty !! \n");
        return(-1);
    } else {
        element = items[front];
        if (front == rear){
            front = -1;
            rear = -1;
        } /* Q has only one element, so we reset the queue after dequeing it. ? */
        else {
            front = (front + 1) % MAX_QUEUE_SIZE;

        }
        printf("\n Deleted element -> %d \n", element);
        return(element);
    }
}
void display()
{
    int i;
    if(isEmpty()) printf(" \n Empty Queue\n");
    else
    {
        printf("\n Front -> %d ",front);
        printf("\n Items -> ");
        for( i = front; i!=rear; i=(i+1)%MAX_QUEUE_SIZE) {
            printf("%d ",items[i]);
        }
        printf("%d ",items[i]);
        printf("\n Rear -> %d \n",rear);
    }
}

void Pthread_mutex_lock(pthread_mutex_t *mutex) {
    int rc = pthread_mutex_lock(mutex);
    assert(rc == 0);
}

void Pthread_mutex_unlock(pthread_mutex_t *mutex) {
    int rc = pthread_mutex_unlock(mutex);
    assert(rc == 0);
}

void Pthread_cond_wait(pthread_cond_t* cv, pthread_mutex_t* mutex) {
    int rc = pthread_cond_wait(cv, mutex);
    assert(rc == 0);
}

void Pthread_cond_signal(pthread_cond_t* cv) {
    int rc = pthread_cond_signal(cv);
    assert(rc == 0);
}