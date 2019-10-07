#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <map>

//typedef  unsigned long long int uint64_t;
//typedef  unsigned long long int uint32_t;
typedef struct _Spaceship Spaceship;
typedef struct _Spaceman Spaceman;
typedef struct __attribute__ ((packed)) _Spaceship {
  char Name[40];
  char LaunchCode[48];
  char AccessCode[8];
  Spaceman * pilot;
  Spaceman * medic;
  Spaceman * engineer;
  Spaceman * Capitan;
} Spaceship;
typedef struct __attribute__ ((packed)) _Spaceman {
  unsigned int name_len;
  unsigned int password_len;
  char Name[60];
  char Password[60];
  _Spaceship * ship;
} Spaceman;

Spaceship * spaceships[32];
bool AllocSpaceship(uint32_t idx){
  spaceships[idx] = (Spaceship*)malloc(sizeof(Spaceship));
  if (spaceships[idx] !=0){
    memset(spaceships[idx],0,sizeof(Spaceship));
    return true;
  }
  return false;
}
bool FreeSpaceship(uint32_t idx){
  if (spaceships[idx]!=0){
    free(spaceships[idx]);
    return true;
  }
  return false;
}
Spaceman * spacemans[32];
bool AllocSpaceman(uint32_t idx){
  //if (spacemans[idx] !=0)// TODO comment this for bug
  //  return false;
  spacemans[idx] = (Spaceman*)malloc(sizeof(Spaceman));
  if (spacemans[idx] !=0){
    //memset(spacemans[idx],0,sizeof(Spaceman));
    return true;
  }
  return false;
}
bool FreeSpaceman(uint32_t idx){
  //printf("LL %016llx\n",(uint64_t)spacemans[idx]);
  if (spacemans[idx]!=0){
    free(spacemans[idx]);
    //spacemans[idx]=0;// TODO comment this for bug
    return true;
  }
  return false;
}
int RemoveCRLF(char * buf){
  char * pos = strstr(buf,"\x0a");
  if (pos != 0){
    *pos = 0;
    return pos-buf;
  }
  return strlen(buf);
}
void FilterDots(char * fname){
  int len = strlen(fname);
  for (int i=0; i<len; ++i){
    if (fname[i] == '.'){
      fname[i] = '_';
    }
  }
}
unsigned int FindFreeSpaceman(){
  for (int i=0; i<32; i++){
    if (spacemans[i] == 0){
      return i;
    }
  }
  return -1;
}
unsigned int FindFreeSpaceship(){
  for (int i=0; i<32; i++){
    if (spaceships[i] == 0){
      return i;
    }
  }
  return -1;
}
bool IsSpaceman(char * name){
  char buf[1024];
  snprintf(buf,1024,"spacemans/%s",name);
  FILE * f = fopen(buf,"r");
  if (f){
    fclose(f);
    return true;
  }
  return false;
}
bool IsSpaceship(char * name){
  char buf[1024];
  snprintf(buf,1024,"spaceships/%s",name);
  FILE * f = fopen(buf,"r");
  if (f){
    fclose(f);
    return true;
  }
  return false;
}
unsigned int LoadSpaceship(char * name,Spaceman * sm);
unsigned int LoadSpaceman(char * name, Spaceship * sh){
  char buf[1024];
  snprintf(buf,1024,"spacemans/%s",name);
  FilterDots(buf);
  FILE * f = fopen(buf,"r");
  if (!f){
    return -1;
  }
  Spaceman * s = (Spaceman *)malloc(sizeof(Spaceman));
  fgets(buf,1024,f);
  if (!strcmp(buf,".")){
    s->name_len = 0;
  }
  else{
    s->name_len = strlen(buf);
    strcpy(&(s->Name[0]),buf);
  }

  fgets(buf,1024,f);
  if (!strcmp(buf,".")){
    s->password_len = 0;
  }
  else{
    s->password_len = strlen(buf);
    strcpy(&(s->Password[0]),buf);
  }

  if (sh == 0){
    fgets(buf,1024,f);
    if (!strcmp(buf,".")){
      s->ship = 0;
    }
    else{
      unsigned int idx_sh = LoadSpaceship(buf,s);
      if (idx_sh ==-1){
        printf("Invalid spaceship at spaceman");fflush(stdout);
      }
      else{
        s->ship = spaceships[idx_sh];
      }
    }
  }
  else{
    s->ship = sh;
  }
  fclose(f);
  unsigned int idx = FindFreeSpaceman();
  spacemans[idx] = s;
  return idx;
}
void StoreSpaceman(Spaceman * sp){
  char buf[1024];
  snprintf(buf,1024,"spacemans/%s",sp->Name);
  FilterDots(buf);
  FILE * f = fopen(buf,"w+");
  if (sp->name_len !=0){
    fputs(sp->Name,f);
  }
  else{
    fputs(".",f);
  }
  fputs("\n",f);
  if (sp->password_len !=0){
    fputs(sp->Password,f);
  }
  else{
    fputs(".",f);
  }
  fputs("\n",f);

  if (sp->ship){
    fputs(sp->ship->Name,f);
  }
  else{
    fputs(".",f);
  }
  fclose(f);
}
unsigned int LoadSpaceship(char * name,Spaceman * sm){
  char buf[1024];
  snprintf(buf,1024,"spaceships/%s",name);
  FilterDots(buf);
  FILE * f = fopen(buf,"r");
  if (!f){
    return -1;
  }
  Spaceship * s = (Spaceship *)malloc(sizeof(Spaceship));
  fgets(buf,1024,f);
  RemoveCRLF(buf);
  if (!strcmp(buf,".")){
  }
  else{
    strcpy(&(s->Name[0]),buf);
  }

  fgets(buf,1024,f);
  RemoveCRLF(buf);
  if (!strcmp(buf,".")){
  }
  else{
    strcpy(&(s->LaunchCode[0]),buf);
  }

  fgets(buf,1024,f);
  RemoveCRLF(buf);
  if (!strcmp(buf,".")){
  }
  else{
    strcpy(&(s->AccessCode[0]),buf);
  }

  fgets(buf,1024,f);
  RemoveCRLF(buf);
  if (!strcmp(buf,".")){// pilot is not presented
    s->pilot = 0;
  }
  else{// pilot is presented
    if (sm!=0 && !strcmp(sm->Name,buf)){
      s->pilot = sm;
    }
    else {
      unsigned int idx_sm = LoadSpaceman(buf,s);
      if (idx_sm ==-1){
        printf("Cannot load pilot %s\n",buf);fflush(stdout);
      }
      else{
        s->pilot = spacemans[idx_sm];
      }
    }
  }

  fgets(buf,1024,f);
  RemoveCRLF(buf);
  if (!strcmp(buf,".")){// medic is not presented
    s->medic = 0;
  }
  else{// medic is presented
    if (sm!=0 && !strcmp(sm->Name,buf)){
      s->medic = sm;
    }
    else {
      unsigned int idx_sm = LoadSpaceman(buf,s);
      if (idx_sm ==-1){
        printf("Cannot load medic %s\n",buf);fflush(stdout);
      }
      else{
        s->medic = spacemans[idx_sm];
      }
    }
  }

  fgets(buf,1024,f);
  RemoveCRLF(buf);
  if (!strcmp(buf,".")){// engineer is not presented
    s->engineer = 0;
  }
  else{// engineer is presented
    if (sm!=0 && !strcmp(sm->Name,buf)){
      s->engineer = sm;
    }
    else{
      unsigned int idx_sm = LoadSpaceman(buf,s);
      if (idx_sm ==-1){
        printf("Cannot load engineer %s\n",buf);fflush(stdout);
      }
      else{
        s->engineer = spacemans[idx_sm];
      }
    }
  }

  fgets(buf,1024,f);
  RemoveCRLF(buf);
  if (!strcmp(buf,".")){// Capitan is not presented
    s->Capitan = 0;
  }
  else{// Capitan is presented
    if (sm!=0 && !strcmp(sm->Name,buf)){
      s->Capitan = sm;
    }
    else {
      unsigned int idx_sm = LoadSpaceman(buf,s);
      if (idx_sm ==-1){
        printf("Cannot load Capitan %s\n",buf);fflush(stdout);
      }
      else{
        s->Capitan = spacemans[idx_sm];
      }
    }
  }
  fclose(f);
  unsigned int idx = FindFreeSpaceship();
  spaceships[idx] = s;
  return idx;
}
void StoreSpaceship(Spaceship * sp){
  char buf[1024];
  snprintf(buf,1024,"spaceships/%s",sp->Name);
  FilterDots(buf);
  FILE * f = fopen(buf,"w+");

  if (strlen(&(sp->Name[0])) !=0){
    fputs(sp->Name,f);
  }
  else{
    fputs(".",f);
  }
  fputs("\n",f);

  if (strlen(&(sp->LaunchCode[0])) !=0){
    fputs(sp->LaunchCode,f);
  }
  else{
    fputs(".",f);
  }
  fputs("\n",f);

  if (strlen(&(sp->AccessCode[0])) !=0){
    fputs(sp->AccessCode,f);
  }
  else{
    fputs(".",f);
  }
  fputs("\n",f);

  if (sp->pilot !=0){
    fputs(sp->pilot->Name,f);
    StoreSpaceman(sp->pilot);
  }
  else{
    fputs(".",f);
  }
  fputs("\n",f);

  if (sp->medic !=0){
    fputs(sp->medic->Name,f);
    StoreSpaceman(sp->medic);
  }
  else{
    fputs(".",f);
  }
  fputs("\n",f);

  if (sp->engineer !=0){
    fputs(sp->engineer->Name,f);
    StoreSpaceman(sp->engineer);
  }
  else{
    fputs(".",f);
  }
  fputs("\n",f);

  if (sp->Capitan !=0){
    fputs(sp->Capitan->Name,f);
    StoreSpaceman(sp->Capitan);
  }
  else{
    fputs(".",f);
  }
  fputs("\n",f);


  fclose(f);
}

int main() {
  char buf[1024];
  //memset(chunk_array,sizeof(char*)*sizeof(chunk_array),0);
  for (int kkk=0; kkk<128; kkk++){
    printf("Commands:\n1.Add spaceship\n2.Add spaceman\n3.Delete spaceship\n4.Delete spaceman\n5.Edit spaceship\n6.View spaceship\n7.View man info\n");
    printf("8.Edit man info\n9.View launch code\n10.Store spaceman\n11.Load spaceman\n12.Search spaceman\n13.Store spaceship\n14.Load spaceship\n15.Search spaceship\n");
    fflush(stdout);
    printf(">");fflush(stdout);
    fgets(buf, 1024, stdin);
    //printf("%s\n",buf);
    uint32_t command = atoi(buf);
    if (command == 1){
      printf("Allocating spaceship at slot:\n");fflush(stdout);
      fgets(buf, 1024, stdin);
      uint32_t idx = atoi(buf);
      if (spaceships[idx] ==0){
        if (AllocSpaceship(idx)){
          printf("Enter name:\n");fflush(stdout);
          fgets(buf, 40, stdin);
          RemoveCRLF(buf);
          strcpy((spaceships[idx]->Name),buf);
          printf("Successfuly allocated at %d\n",idx);fflush(stdout);
        }
        else{
          printf("Cannot allocate at %d\n",idx);fflush(stdout);
        }
      }
      else{
        printf("Already allocated\n");fflush(stdout);
      }
    }
    else if (command == 2){
      printf("Allocating spaceman at slot:\n");fflush(stdout);
      fgets(buf, 1024, stdin);
      uint32_t idx = atoi(buf);
      if (AllocSpaceman(idx)){
        printf("Enter name:\n");fflush(stdout);
        fgets(buf, 60, stdin);
        RemoveCRLF(buf);
        if (strlen(buf) != 0){
          spacemans[idx]->name_len = strlen(buf);
          strcpy(spacemans[idx]->Name,buf);
        }
        printf("Enter password:\n");fflush(stdout);
        fgets(buf, 60, stdin);
        RemoveCRLF(buf);
        if (strlen(buf) !=0){
          spacemans[idx]->password_len = strlen(buf);
          strcpy(spacemans[idx]->Password,buf);
        }
        printf("Successfuly allocated at %d\n",idx);fflush(stdout);
      }
      else{
        printf("Cannot allocate at %d\n",idx);fflush(stdout);
      }
    }
    else if (command == 3){
      printf("Delete spaceship at:\n");fflush(stdout);
      fgets(buf, 1024, stdin);
      uint32_t idx = atoi(buf);
      if (FreeSpaceship(idx)){
        printf("Successfuly deleted spaceship at %d\n",idx);fflush(stdout);
      }
      else{
        printf("Cannot free ship at %d\n",idx);fflush(stdout);
      }
    }
    else if (command == 4){
      printf("Delete spaceman at:\n");fflush(stdout);
      fgets(buf, 1024, stdin);
      uint32_t idx = atoi(buf);
      if (FreeSpaceman(idx)){
        printf("Successfuly deleted spaceman at %d\n",idx);fflush(stdout);
      }
      else{
        printf("Cannot free man at %d\n",idx);fflush(stdout);
      }
    }
    else if (command == 5){
      printf("Enter spaceship slot to edit:\n");fflush(stdout);
      fgets(buf, 1024, stdin);
      uint32_t idx = atoi(buf);
      // Changing ship name
      printf("Enter ship name (blank to leave unchanged):\n");fflush(stdout);
      fgets(buf, 40, stdin);
      RemoveCRLF(buf);
      if (strlen(buf)!=0){
        strcpy(spaceships[idx]->Name,buf);
      }
      // Changing ship launch code
      printf("Enter ship launch code (blank to leave unchanged):\n");fflush(stdout);
      fgets(buf, 48, stdin);
      RemoveCRLF(buf);
      if (strlen(buf)!=0){
        strcpy(spaceships[idx]->LaunchCode,buf);
        printf("Changed to %s\n",spaceships[idx]->LaunchCode);fflush(stdout);
      }
      else{
        printf("Unchanged\n");fflush(stdout);
      }

      // Changing ship launch code
      printf("Enter ship access code (blank to leave unchanged):\n");fflush(stdout);
      fgets(buf, 1024, stdin);
      RemoveCRLF(buf);
      if (strlen(buf)!=0){
        strcpy(spaceships[idx]->AccessCode,buf);
        printf("Changed to %s\n",spaceships[idx]->AccessCode);fflush(stdout);
      }
      else{
        printf("Unchanged\n");fflush(stdout);
      }

      // Changing ship pilot
      printf("Enter ship pilot idx (blank to leave unchanged):\n");fflush(stdout);
      fgets(buf, 1024, stdin);
      RemoveCRLF(buf);
      int idx_man = atoi(buf);
      if (strlen(buf)!=0){
        if (spacemans[idx_man]!=0){
          spaceships[idx]->pilot = spacemans[idx_man];
          spacemans[idx_man]->ship = spaceships[idx];
          printf("Successfuly set pilot %d\n",idx_man);fflush(stdout);
        }
        else{
          printf("Invalid man idx\n");fflush(stdout);
        }
      }
      // Changing ship medic
      printf("Enter ship medic idx (blank to leave unchanged):\n");fflush(stdout);
      fgets(buf, 1024, stdin);
      RemoveCRLF(buf);
      idx_man = atoi(buf);
      if (strlen(buf)!=0){
        if (spacemans[idx_man]!=0){
          spaceships[idx]->medic = spacemans[idx_man];
          spacemans[idx_man]->ship = spaceships[idx];
          printf("Successfuly set medic %d\n",idx_man);fflush(stdout);
        }
        else{
          printf("Invalid man idx\n");fflush(stdout);
        }
      }
      // Changing ship engineer
      printf("Enter ship engineer idx (blank to leave unchanged):\n");fflush(stdout);
      fgets(buf, 1024, stdin);
      RemoveCRLF(buf);
      idx_man = atoi(buf);
      if (strlen(buf)!=0){
        if (spacemans[idx_man]!=0){
          spaceships[idx]->engineer = spacemans[idx_man];
          spacemans[idx_man]->ship = spaceships[idx];
          printf("Successfuly set engineer %d\n",idx_man);fflush(stdout);
        }
        else{
          printf("Invalid man idx\n");fflush(stdout);
        }
      }
      // Changing ship Captitan
      printf("Enter ship Capitan idx (blank to leave unchanged):\n");fflush(stdout);
      fgets(buf, 1024, stdin);
      RemoveCRLF(buf);
      idx_man = atoi(buf);
      if (strlen(buf)!=0){
        if (spacemans[idx_man]!=0){
          spaceships[idx]->Capitan = spacemans[idx_man];
          spacemans[idx_man]->ship = spaceships[idx];
          printf("Successfuly set capitan %d\n",idx_man);fflush(stdout);
        }
        else{
          printf("Invalid man idx\n");fflush(stdout);
        }
      }
      printf("Recorded\n");fflush(stdout);
    }
    else if (command == 6){
      printf("View ship info:\nEnter ship idx:\n");fflush(stdout);
      fgets(buf, 1024, stdin);
      uint32_t idx = atoi(buf);
      printf("%llx\n",spaceships[idx]);fflush(stdout);
      if (spaceships[idx] != 0){
        printf("Name: %s\n",&(spaceships[idx]->Name));fflush(stdout);
        printf("Acc: %s\n",&(spaceships[idx]->AccessCode));fflush(stdout);
        if (spaceships[idx]->pilot != 0){
          printf("Pilot: %s\n",&(spaceships[idx]->pilot->Name));fflush(stdout);
        }
        if (spaceships[idx]->medic != 0){
          printf("Medic: %s\n",&(spaceships[idx]->medic->Name));fflush(stdout);
        }
        if (spaceships[idx]->engineer != 0){
          printf("Engineer: %s\n",&(spaceships[idx]->engineer->Name));fflush(stdout);
        }
        if (spaceships[idx]->Capitan != 0){
          printf("Capitan: %s\n",&(spaceships[idx]->Capitan->Name));fflush(stdout);
        }
      }
      else{
        printf("No such spaceship");fflush(stdout);
      }
    }
    else if (command == 7){
      printf("View man info:\nEnter man idx:\n");fflush(stdout);
      fgets(buf, 1024, stdin);
      //printf("!!!!!!!!!!\n");fflush(stdout);
      uint32_t idx = atoi(buf);
      //printf("%llx\n",spacemans[idx]);fflush(stdout);
      if (spacemans[idx] != 0){
        printf("Name: %s\n",&(spacemans[idx]->Name[0]));fflush(stdout);
        //printf("%llx\n",spacemans[idx]->ship);fflush(stdout);
        if (spacemans[idx]->ship != 0){
          printf("Ship: %s\n",&(spacemans[idx]->ship->Name));fflush(stdout);
          if (spacemans[idx]->ship->pilot != 0 &&
                spacemans[idx]->ship->pilot==spacemans[idx]){
              printf("pilot\n");fflush(stdout);
          }
          if (spacemans[idx]->ship->medic != 0 &&
                spacemans[idx]->ship->medic==spacemans[idx]){
              printf("medic\n");fflush(stdout);
          }

          if (spacemans[idx]->ship->engineer != 0 &&
                spacemans[idx]->ship->engineer==spacemans[idx]){
              printf("engineer\n");fflush(stdout);
          }
          if (spacemans[idx]->ship->Capitan != 0 &&
                spacemans[idx]->ship->Capitan==spacemans[idx]){
              printf("Capitan\n");fflush(stdout);
          }
        }
      }
      else{
        printf("No such spaceman");fflush(stdout);
      }
    }
    else if (command == 8){
      printf("Edit man info. Enter idx:\n");fflush(stdout);
      fgets(buf, 1024, stdin);
      uint32_t idx = atoi(buf);
      if (spacemans[idx] != 0){
        printf("Enter name:\n");fflush(stdout);
        fgets(spacemans[idx]->Name, spacemans[idx]->name_len+2, stdin);
        RemoveCRLF(spacemans[idx]->Name);
        //strcpy(spacemans[idx]->Name,buf);
        printf("Successfuly edited name to %s\n",spacemans[idx]->Name);fflush(stdout);
        printf("Enter password:\n");fflush(stdout);
        fgets(spacemans[idx]->Password, spacemans[idx]->password_len+2, stdin);
        RemoveCRLF(spacemans[idx]->Password);
        printf("Successfuly edited password to %s\n",buf);fflush(stdout);
      }
    }
    else if (command == 9){
      printf("View launch code of ship. Enter idx:\n");fflush(stdout);
      fgets(buf, 1024, stdin);
      uint32_t idx = atoi(buf);
      if (spaceships[idx] != 0){
        printf("Enter access code:\n");fflush(stdout);
        fgets(buf, 1024, stdin);
        RemoveCRLF(buf);
        if (!strcmp(buf,&(spaceships[idx]->AccessCode[0]))){
          printf("Launch code: %s\n",&(spaceships[idx]->LaunchCode[0]));fflush(stdout);
        }
        else{
          printf("Invalid access code %s\n",&(spaceships[idx]->AccessCode[0]));fflush(stdout);
        }
      }
      else{
        printf("No spaceship at %s\n",buf);fflush(stdout);
      }
    }
    else if (command == 10){
      // Storing spaceman
      printf("Storing spaceman. Enter idx:\n");fflush(stdout);
      fgets(buf, 1024, stdin);
      uint32_t idx = atoi(buf);
      if (spacemans[idx] != 0){
        StoreSpaceman(spacemans[idx]);
      }
      else{
        printf("No such spaceman\n");fflush(stdout);
      }
    }
    else if (command == 11){
      printf("Loading spaceman. Enter name:\n");fflush(stdout);
      fgets(buf, 1024, stdin);
      RemoveCRLF(buf);
      unsigned int idx = LoadSpaceman(buf,0);
      if (idx != -1){
        printf("Loaded spaceman at %d\n",idx);fflush(stdout);
      }
      else{
        printf("No such spaceman\n");fflush(stdout);
      }
    }
    else if (command == 12){
      printf("Searching the spaceman\n");
      printf("Enter name pattern:\n");fflush(stdout);
      fgets(buf, 1024, stdin);
      RemoveCRLF(buf);
      DIR *dir;
      struct dirent *ent;
      if ((dir = opendir ("spacemans")) != NULL) {
          while ((ent = readdir (dir)) != NULL) {
            if (strstr(ent->d_name,buf)){
              printf ("%s\n", ent->d_name);fflush(stdout);
            }
          }
          closedir (dir);
      } else {
        printf("No spaceman dir found\n");fflush(stdout);
      }
    }
    else if (command == 13){
      // Storing spaceship
      printf("Storing spaceship. Enter idx:\n");fflush(stdout);
      fgets(buf, 1024, stdin);
      uint32_t idx = atoi(buf);
      if (spaceships[idx] != 0){
        if (IsSpaceship(spaceships[idx]->Name)){
          printf("Spaceship already exists\n");fflush(stdout);
        }
        else{
          StoreSpaceship(spaceships[idx]);
        }
      }
      else{
        printf("No such spaceship\n");fflush(stdout);
      }
    }
    else if (command == 14){
      printf("Loading spaceship. Enter name:\n");fflush(stdout);
      fgets(buf, 1024, stdin);
      RemoveCRLF(buf);
      unsigned int idx = LoadSpaceship(buf,0);
      if (idx != -1){
        printf("Loaded spaceship at %d\n",idx);fflush(stdout);
      }
      else{
        printf("No such spaceship\n");fflush(stdout);
      }
    }
    else if (command == 15){
      printf("Searching the spaceship\n");
      printf("Enter name pattern:\n");fflush(stdout);
      fgets(buf, 1024, stdin);
      RemoveCRLF(buf);
      DIR *dir;
      struct dirent *ent;
      if ((dir = opendir ("spaceships")) != NULL) {
          while ((ent = readdir (dir)) != NULL) {
            if (strstr(ent->d_name,buf)){
              printf ("%s\n", ent->d_name);fflush(stdout);
            }
          }
          closedir (dir);
      } else {
        printf("No spaceship dir found\n");fflush(stdout);
      }
    }
    else if (command == 254){
      printf("Spaceships:\n");fflush(stdout);fflush(stdout);
      for (int i=0; i<32; i++){
        printf("%016llX\n",spaceships[i]);fflush(stdout);
      }
      fflush(stdout);
    }
    else if (command == 255){
      printf("Spacemans:\n");
      for (int i=0; i<32; i++){
        printf("%016llX\n",spacemans[i]);
      }
      fflush(stdout);
    }
    else if (command == 256){
      printf("Enter spaceship idx:\n");
      fgets(buf, 1024, stdin);
      uint32_t idx = atoi(buf);
      if (spaceships[idx] != 0){
        uint64_t * rr = (uint64_t *)(((char*)&spaceships[idx])-16);
        for (int ii=0; ii<8; ii++){
          printf("%016llx: %016llx\n",&rr[ii],rr[ii]);
        }
        puts("");fflush(stdout);
      }
    }
    else if (command == 257){
      printf("Enter addr:\n");fflush(stdout);
      fgets(buf, 1024, stdin);
      uint64_t adr = strtoull(buf,0,10);
      printf("Enter qwcount:\n");fflush(stdout);
      fgets(buf, 1024, stdin);
      uint64_t qwc = strtoull(buf,0,10);
      uint64_t * rr = (uint64_t *)adr;
      for (int ii=0; ii<qwc; ii++){
        printf("%016llx: %016llx\n",&rr[ii],rr[ii]);
      }
      puts("");fflush(stdout);
    }
    else{
      printf("Invalid command number %d\n",command);;fflush(stdout);
    }
  }
  return 0;
}
