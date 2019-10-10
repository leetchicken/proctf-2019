#include <unistd.h>

int main(){
  char * args [2];
  char * cmd = "/bin/ls";
  args[0] = cmd;
  args[1] = 0;
  execve(cmd,args,0);
  return 0;
}
