#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_LETTER_SLOT 8ULL
#define MAX_LETTER_SIZE 0x10000ULL

char *letter[MAX_LETTER_SLOT];

/**
 * Utility functions
 */
size_t readline(const char *msg, char *buf, size_t size) {
  printf("%s", msg);
  memset(buf, 0, size);
  for (size_t i = 0; i < size - 1; i++) {
    if (read(STDIN_FILENO, buf + i, 1) != 1) {
      exit(1);
    } else if (buf[i] == '\n') {
      buf[i] = '\0';
      break;
    }
  }
}

int getint(const char *msg) {
  char buf[0x10];
  readline(msg, buf, sizeof(buf));
  return atoi(buf);
}

/**
 * Write letter
 */
void letter_new() {
  size_t index, size;

  // Get index and size
  if ((index = (size_t)getint("Index: ")) >= MAX_LETTER_SLOT)
    return;
  if ((size = (size_t)getint("Size: ")) > MAX_LETTER_SIZE)
    return;

  // Allocate buffer for letter
  if (!(letter[index] = (char*)malloc(size + 1)))
    return;

  // Get content
  readline("String: ", letter[index], size + 1);
}

/**
 * Redact letter
 */
void letter_blackout() {
  char word[0x20], *wordptr;
  size_t index, wordlen, letterlen;

  // Get and check index
  if ((index = (size_t)getint("Index: ")) >= MAX_LETTER_SLOT
      || !letter[index])
    return;
  letterlen = strlen(letter[index]);

  // Get word to redact
  readline("Word to redact: ", word, sizeof(word));
  if ((wordlen = strlen(word)) == 0)
    return;

  // Blackout
  wordptr = letter[index];
  while ((wordptr - letter[index] < letterlen)
         && (wordptr = memmem(wordptr, letter[index] + letterlen - wordptr,
                              word, wordlen))) {
    memset(wordptr, '*', wordlen);
    wordptr += wordlen;
  }

  printf("[Redacted]\n%s\n", letter[index]);
}

/**
 * Delete letter
 */
void letter_delete() {
  size_t index;

  // Get and check index
  if ((index = (size_t)getint("Index: ")) >= MAX_LETTER_SLOT
      || !letter[index])
    return;

  // Free buffer
  free(letter[index]);
  letter[index] = NULL;
}

/**
 * Entry point
 */
int main() {
  setvbuf(stdin, NULL, _IONBF, 0);
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);

  puts("1. Write\n" "2. Blackout\n" "3. Discard");
  while (1) {
    switch (getint("> ")) {
      case 1: letter_new(); break;
      case 2: letter_blackout(); break;
      case 3: letter_delete(); break;
      default: return 0;
    }
  }
}
