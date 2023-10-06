#include <stdio.h>
#include <string.h>

#define BEGIN_OBFUSCATE(N) \
  void *__next[(N)+1];     \
  unsigned __n = 0;
#define END_OBFUSCATE     \
  __next[__n] = &&__end;  \
  __n = 0;                \
  goto *__next[__n++];    \
__end:
#define EXEC(stmt)        \
  {                       \
    __label__ skip, here; \
    goto skip;            \
  here:                   \
    stmt;                 \
    goto *__next[__n++];  \
  skip:                   \
  __next[__n++] = &&here; \
  }


unsigned char ans[] = "\xf0\xe4\x25\xdd\x9f\x0b\x3c\x50\xde\x04\xca\x3f\xaf\x30\xf3\xc7\xaa\xb2\xfd\xef\x17\x18\x57\xb4\xd0\x8f\xb8\xf4\x23";
unsigned char key[] = "\xf6\xf5\x31\xc8\x81\x15\x14\x68\xf6\x35\xe5\x3e\x82\x09\xca\xf1\x8a\xa9\xdf\xdf\x33\x2a\x6d\x81\xf5\xa6\x85\xdf\x17";

unsigned char enc(unsigned char c, int i) {
  BEGIN_OBFUSCATE (5);
  unsigned char o, k;

  EXEC( o = c );
  EXEC( o ^= i );
  EXEC( o ^= 0x55 );
  EXEC( k = key[i] );
  EXEC( o ^= k );

  END_OBFUSCATE;
  return o;
}

int check(unsigned char *flag) {
  BEGIN_OBFUSCATE (3);
  int ok;

  EXEC( if (strlen(flag) != 29) return 1; );
  EXEC( ok = 1; );
  EXEC(
    for (int i = 0; i < 29; i++) {
      ok &= enc(flag[i], i) == ans[i];
    }
  );

  END_OBFUSCATE;
  return !ok;
}

int main() {
  BEGIN_OBFUSCATE (5);
  int r;
  unsigned char flag[100];

  EXEC( memset(flag, 0, sizeof(flag)); );
  EXEC( printf("FLAG: "); );
  EXEC( if (scanf("%99s", flag) != 1) return 1; );
  EXEC( r = check(flag); );
  EXEC(
    if (r == 0)
      puts("Correct!");
    else
      puts("Wrong...");
  );

  END_OBFUSCATE;
  return 0;
}
