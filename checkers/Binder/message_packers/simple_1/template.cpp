#include <stdio.h>
#include <string.h>

int coded_mes_len = MESLEN;
char coded_mes [] =  MES ;
char valid_key = KEY;

int main (int argc,char * argv[]){
  if (argc <2){
    return 1;
  }
  char key = 0;
  int len = strlen(argv[1]);
  for (int i=0; i<len; i++){
    key = key ^ argv[1][i];
  }
  if (key != valid_key){
    puts("Invalid key\n");
    return 0;
  }
  for (int i=0; i<coded_mes_len; i++){
    coded_mes[i] = coded_mes[i] ^ key;
  }
  printf("%s\n",coded_mes);
  return 0;
}
