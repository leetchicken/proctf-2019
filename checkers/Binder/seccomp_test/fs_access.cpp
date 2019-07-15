#include <stdio.h>
#include <stdlib.h>

int main(){
  FILE * f = fopen("1.txt","w+");
  fputs("qwe\n",f);
  fclose(f);
  return 0;
}
