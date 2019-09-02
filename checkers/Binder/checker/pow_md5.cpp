#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <openssl/md5.h>

unsigned char result[MD5_DIGEST_LENGTH];

// Print the MD5 sum as hex-digits.
void print_md5_sum(unsigned char* md) {
    int i;
    for(i=0; i <MD5_DIGEST_LENGTH; i++) {
            printf("%02x",md[i]);
    }
}

void gen_random(char *s, const int len) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < len; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }
    s[len] = 0;
}
int main(int argc, char *argv[]) {
    char buf[17];
    unsigned char result[MD5_DIGEST_LENGTH];
    if (argc != 2){
      printf("No salt provided\n");
      return 0;
    }
    if (strlen(argv[1]) != 8){
      printf("Salt must be 8 chars\n");
      return 0;
    }
    srand (time(NULL));
    strcpy(buf,argv[1]);
    int i;
    while (1){
      gen_random(buf+8,8);
      //printf("%s\n",buf);
      MD5((const unsigned char*)buf, 16, result);
      unsigned int val = *((unsigned int *) result);
      if (val > 0xFFFFFaa0){
        //printf("Val: %x %x\n",val,0xF0000000);
        //print_md5_sum(result);
        break;
      }
    }
    printf("%s\n",buf+8);
    //print_md5_sum(result);
    //puts("");
    return 0;
}
