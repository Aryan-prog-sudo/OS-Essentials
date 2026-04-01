#include "loader.h"
//All the necessary headers are included in the loader.h file and there is no need to include them here

Elf32_Ehdr *ehdr = NULL; //how the entire file is layed out
Elf32_Phdr *phdr = NULL; //describes a segment.(contains info about loading)
int fd;
long elf_base_address = 0;
//The Elf32_Ehdr and the Elf32_Phdr are in the elf.h header file

//To cleanup the program
void loader_cleanup(){
  close(fd);
}

void load_and_run_elf(char** exe) {
  fd = open(exe[1], O_RDONLY);
  //O_RDONLY mens read only as we only need to open and read it into memory.
  if(fd==-1){
    perror("Failed in opening\n %s"); //(Aryan)perror only takes one agument that is a string, was given an argument exe[1] ?
    return;
  }

  //Added semicolons
  off_t filesize=lseek(fd,0,SEEK_END);
  //off_t data type is used to represent filesize or offset, lseek moves the file pointer within a file here offset is zero and it say that move to the end of the file 
  //it returns the new file offset (in bytes from the beginning of the file).
  
  lseek(fd,0,SEEK_SET);
  //reset to the begining
  //Allocate memory on the heap to hold the entire file
  
  void* ELF_Data = malloc(filesize);
  if(ELF_Data==NULL){
    perror("Memory allocation failed for ELF_Data");
  }
  //ELF_Data is a pointer to memory buffer, where the data will be stored!(it must be big enough to hold filesize)
  // Read the file contents into memory
  //ssize_t is the number of bites actually read it might be less then count(3rd parameter in read())
 
  ssize_t bytes_read = read(fd, ELF_Data, filesize);
  assert(bytes_read == filesize);  // Ensure we read the full file
  if (bytes_read < 0) {
    perror("couldn't read the file");
    close(fd);
    return;
    }

  
  //ELF Magic number to check the validity of the elf file.
  ehdr = (Elf32_Ehdr*) ELF_Data;
  //we can check if this elf file is a valid file or not by magic numbers!!
  // The first 4 bytes of every ELF file are a fixed signature called the magic number.
  unsigned char *e_ident = ehdr->e_ident;
  if (e_ident[0] == 0x7F && e_ident[1] == 'E' && e_ident[2] == 'L' && e_ident[3] == 'F') {
        printf("Valid ELF file!\n");
    } else {
        perror("Invalid ELF file!\n");
    }
  
  // 2. Iterate through the PHDR table and find the section of PT_LOAD type that contains the address of the entrypoint method in fib.c
  phdr=(Elf32_Phdr*)((char*)ehdr + ehdr->e_phoff); 
  //Changed ELF32_Phdr to Elf32_Phdr also added semicolon
  int SegmentNumber = ehdr->e_phnum;//number of program headers
  Elf32_Addr EntryAddress = ehdr->e_entry;//entry point address
  void* Entry = NULL;//entry point address //R
  unsigned short phentsize=ehdr->e_phentsize;
  //size of each program header
  //loop through each header
  for(int i=0;i<SegmentNumber;i++){
    if(phdr[i].p_type==PT_LOAD){
      //load in memory
      void* VirtualMemory = mmap(NULL, phdr[i].p_memsz, PROT_READ | PROT_WRITE | PROT_EXEC,MAP_PRIVATE | MAP_ANONYMOUS,0, 0);
      assert(VirtualMemory != MAP_FAILED);
      // lseek(fd, phdr[i].p_offset, SEEK_SET);
      // read(fd, VirtualMemory, phdr[i].p_memsz);
      memcpy(VirtualMemory, ELF_Data + phdr[i].p_offset, phdr[i].p_memsz);

      printf("Mapped segment %d to address %p with size %u bytes.\n", i, VirtualMemory, phdr[i].p_memsz);
      
      if(phdr[i].p_vaddr<=EntryAddress && EntryAddress<phdr[i].p_vaddr+phdr[i].p_memsz){
        Entry=(void*)((char*)VirtualMemory+(EntryAddress-phdr[i].p_vaddr));
        printf("Entry point found! Original virtual address: 0x%x, Mapped physical address: %p.\n", EntryAddress, Entry);
        // Changed memsz and vaddr to p_memsz and p_vaddr
      }
    }
    
  }
  printf("Loop teminated\n");
  
  if (Entry != NULL) {
    int (*_start)() = (int (*)()) Entry;  // entry point returns int(we have the entry point addres we type casted it to a function so that we can call it)
    int result = _start();                // call _start and get return value
  printf("User _start return value = %d\n",result);
  } else {
      fprintf(stderr, "Entry point not found!\n");
  }
  /*“We have a void * pointer to the start of the binary's execution (the _start() function).
We cast it to a function pointer so we can actually call it like a normal function, and that runs the program.”*/


  
  
    /*here our ehdr is for 32 bytes if we do ehdr+1 it will add 332 bytes to the inital header we have
    typecased it to char type so if you do ehdr+1 it will only move by 1 byte*/
    
  // 3. Allocate memory of the size "p_memsz" using mmap function 
  //    and then copy the segment content
  // 4. Navigate to the entrypoint address into the segment loaded in the memory in above step
  // 5. Typecast the address to that of function pointer matching "_start" method in fib.c.
  // 6. Call the "_start" method and print the value returned from the "_start"
  if(ELF_Data){
    free(ELF_Data);
  }
}

int main(int argc, char **argv){
    if(argc<2){
        printf("Missing ELF file");
        return 1;
    }
    load_and_run_elf(argv);
    loader_cleanup();
}