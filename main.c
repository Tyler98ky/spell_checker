#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utility.h"

#define MAX_ARR_SIZE 2048
#define MAX_DICTIONARY_WORD_SIZE 100
#define MAX_DICTIONARY_WORD_COUNT 110000  // 110,000
#define DEFAULT_PORT 4444
#define DEFAULT_DICTIONARY "words"


FILE* DictionaryUsed;
int PortUsed;
char* WordBank[MAX_DICTIONARY_WORD_COUNT];

void initialSetup(int argc, char *const *argv);
int isValidWord(const char* word);
void setupWordBank(void);
void printDictionary();

int main(int argc, char *argv[]) {
    initialSetup(argc, argv);
    setupWordBank();

    printf("Dictionary: %p Port Number: %d\n", DictionaryUsed, PortUsed);

    return 0;
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
        buffer[strlen(buffer)-1] = 0;  // Remove newline characters
        WordBank[i] = strdup(buffer);
        i++;

    }
}

int isValidWord(const char* word) {
    size_t wordBankSize = sizeof(WordBank) / sizeof(WordBank[0]);

    for (size_t i = 0; i < wordBankSize; i++) {
        if (WordBank[i] == NULL) break;  // End of dictionary

        if (!strcmp(WordBank[i], word)) {
            return 1;
        }
    }
    return 0;
}