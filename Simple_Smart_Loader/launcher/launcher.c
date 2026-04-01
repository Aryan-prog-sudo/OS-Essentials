#include "loader.h"

int main(int argc, char** argv){
  if(argc!=2){
    fprintf(stderr, "Missoing arguements");
    printf("Should contain the launch and the test file");
    return EXIT_FAILURE;
  }


  FILE* FilePointer = fopen(argv[1], "rb");
  if(FilePointer==NULL){
    fprintf(stderr, "File could not open");
    return EXIT_FAILURE;
  }
  fclose(FilePointer);

  load_and_run_elf(argv);
  loader_cleanup();
}