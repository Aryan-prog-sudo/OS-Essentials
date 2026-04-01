#include "loader.h"

//All the global variables
#define PAGE_SIZE 4096 // this is the general size of a page in a linux (4 KB)
Elf32_Ehdr *ehdr = NULL; // ELF Header
Elf32_Phdr *phdr = NULL; // Program/Segment Header
int fd; // File descriptor
long elf_base_address = 0;
void* ELF_Data = NULL; // its a blue print of entire ELF file in fat RAM fro fast access
long num_of_page_faults = 0; //This counts total page faults 
long NumPageAllocated = 0; //This counts page allocated for handling page fault
long TotalMemory=0;
unsigned long TotalBytesMapped = 0; //Tracks the actual useful bytes on the pages we allocate.


//To cleanup the program
void loader_cleanup() {
  printf("Total Page Faults handled: %ld\n", num_of_page_faults);
  printf("Total Pages Allocated:     %ld\n", NumPageAllocated);
  
  // This local variable will store the actual fragmentation.
  unsigned long Internal = 0;
  
  // Calculate total bytes allocated by the OS
  TotalMemory = NumPageAllocated * PAGE_SIZE;
  // Actual fragmentation is the wasted space only in the pages we allocated.
  if (TotalMemory > TotalBytesMapped) {
    Internal = TotalMemory - TotalBytesMapped;
  }
  
  //Convert fragmentation in KB
  //Divide by 1024 to conver in KB
  printf("Total Internal Fragmentation: %f KB\n", (double)Internal / 1024.0);
      
  // Free the memory
  if(ELF_Data) {
    free(ELF_Data);
    ELF_Data = NULL;
  }
  
  // Close the file descriptor
  if (fd != -1) {
    close(fd);
    fd = -1;
  }
}


//Custom Signal Handler
//Page fault handler we will run this whenever segmentation fault (SIGSEGV) occurs catch this fault allocate the memory then resume
void crash_handler(int sig, siginfo_t *information_about_signal, void *ucontext ) {
  void* page_fault_virtual_addr = information_about_signal->si_addr;
  
  //we need to round DOWN the fault address to the nearest multiple of 4096
  
  void* page_start = (void*)(((unsigned long)page_fault_virtual_addr) & ~((PAGE_SIZE - 1)));
  // void* page_start = (void*)(((unsigned long)page_fault_virtual_addr / PAGE_SIZE) * PAGE_SIZE);
  
  num_of_page_faults++; //Everytime the CrashHandler is called that means there is a page fault
  
  printf("page fault has occured at address: %p\n", information_about_signal->si_addr);
  //This prints the exact virtual address that program tried to access
  Elf32_Phdr* segment = NULL;
  for(int i = 0; i < ehdr->e_phnum; i++) { //find all the loadable segements ie segments with the p_type PT_LOAD
    if(phdr[i].p_type == PT_LOAD) {
      //get the start and the end of the virtual address of lodable segment
      unsigned long segment_start = phdr[i].p_vaddr;
      unsigned long segment_end = segment_start + phdr[i].p_memsz; //(we used memsz insted of filesz because its larger make room for .bss)
      if((unsigned long)page_fault_virtual_addr >= segment_start && (unsigned long)page_fault_virtual_addr < segment_end) {
        segment = &phdr[i];
        break;
      }
    }
  }
  
  if(segment == NULL) {
    fprintf(stderr, "Real Segmentation Fault! Address %p is not valid.\n", page_fault_virtual_addr);
    exit(1);
    //This means that the address tried to access is not a valid address
    //There is a real bug ie NULL pointer or some other erroe
  }
  //when you do page size -1 you get 4095 which in 32 bit system have 12 ones at the end
  //when you ~ it you get 12 zeros at the end and when done and with fault address it makes end 12 bits of the fault addres xero and a multipe of page size
  
  void *new_page = mmap(page_start, PAGE_SIZE, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED, -1, 0);
  //here we are allocating the actual memory at page_start that we calculated of 1 page size and gave it all the permissions,private and anonymous menas its not backed by any file is fresh RAM
  //fixed tells mmap don't find any different addres use page_start
  if(new_page == MAP_FAILED) {
    perror("mmap failed in crash handler");
    exit(1);
  }
  NumPageAllocated++;

  // Calculate the useful bytes on this newly mapped page
  unsigned long PageStartAddress = (unsigned long)page_start;
  unsigned long PageEndAddress = PageStartAddress + PAGE_SIZE;
  unsigned long SegmentStartAddress = segment->p_vaddr;
  unsigned long SegmentEndAddress = SegmentStartAddress + segment->p_memsz;
  
  // Find the overlap between the page and the segment's memsz
  unsigned long StartOverlap;
  unsigned long EndOverlap;
  //The start of overlap is basically the maximum of allocated page address and the segment start address
  //The end of overlap is basically the minimum of the allocated page address and the segment end address
  
  //For start of overlap(Basically a max function)
  if (PageStartAddress > SegmentStartAddress) {
    StartOverlap = PageStartAddress;
  }
  else {
    StartOverlap = SegmentStartAddress;
  } 

  //For End Of overlap(Basically a min function)
  if(PageEndAddress<SegmentEndAddress){
    EndOverlap = PageEndAddress;
  }
  else{
    EndOverlap = SegmentEndAddress;
  }


  unsigned long useful_bytes_on_this_page = 0;
  if (EndOverlap > StartOverlap) {
    useful_bytes_on_this_page = EndOverlap - StartOverlap;
  }
  
  // Add this page's useful bytes to the global total
  TotalBytesMapped += useful_bytes_on_this_page;
  //now we will copy data from our elf_data into our new page
  unsigned long page_offset_in_segment = (unsigned long)page_start - segment->p_vaddr;
  //how far into the segmeny is the page we are trying to load
  unsigned long file_offset_for_page = segment->p_offset + page_offset_in_segment;
  //this tells where is the specific data in ELF file
  unsigned long segment_size = segment->p_filesz;
  //how much data we actually need to copy
  unsigned long copy_size = 0;
  if(page_offset_in_segment < segment_size) {
    if(segment_size - page_offset_in_segment > PAGE_SIZE) {
      copy_size = PAGE_SIZE; //copy a full page
    } 
    else{
      copy_size = segment_size - page_offset_in_segment;
      //copy the last page
    }
    memcpy(page_start, (char*)ELF_Data + file_offset_for_page, copy_size);
  }
  return;
}


//Loads the elf
void load_and_run_elf(char** exe) {
  fd = open(exe[1], O_RDONLY);
  //O_RDONLY mens read only as we only need to open and read it into memory.
  if(fd == -1) {
    perror("Failed in opening file"); // MODIFIED: Corrected perror call
    return;
  }

  //Added semicolons
  off_t Filesize = lseek(fd, 0, SEEK_END);
  //off_t data type is used to represent filesize or offset, 
  //lseek moves the file pointer within a file here offset is zero and it say that move to the end of the file 
  //it returns the new file offset (in bytes from the beginning of the file).
  
  lseek(fd, 0, SEEK_SET);
  //SEEK_SET moves to the start of the file that is pointed to using the fd file descriptor
  //reset to the begining
  //Allocate memory on the heap to hold the entire file
  
  ELF_Data = malloc(Filesize);
  if(ELF_Data == NULL) {
    perror("Memory allocation failed for ELF_Data");
    close(fd); // ADDED: Cleanup on error
    return;
  }
  
  //ELF_Data is a pointer to memory buffer, where the data will be stored!(it must be big enough to hold filesize)
  // Read the file contents into memory
  //ssize_t is the number of bites actually read it might be less then count(3rd parameter in read())
  ssize_t bytes_read = read(fd, ELF_Data, Filesize); //It makes a "photocopy" of the entire ELF file into RAM first.
  
  //(Change by Mayank) Check for read error properly
  if (bytes_read != Filesize) { 
    perror("couldn't read the full file");
    close(fd);
    free(ELF_Data);
    return;
  }
  // assert(bytes_read == Filesize)

  //ELF Magic number to check the validity of the elf file.
  ehdr = (Elf32_Ehdr*) ELF_Data;
  //we can check if this elf file is a valid file or not by magic numbers!!
  // The first 4 bytes of every ELF file are a fixed signature called the magic number.
  unsigned char *MagicNumber = ehdr->e_ident;
  if(MagicNumber[0] == 0x7F && MagicNumber[1] == 'E' && MagicNumber[2] == 'L' && MagicNumber[3] == 'F') {
    printf("Valid ELF file!\n");
  } 
  else {
    fprintf(stderr, "Invalid ELF file!\n"); //(Changed By Mayank) Use fprintf for errors
    close(fd);
    free(ELF_Data);
    return;
  }
  
  //Iterate through the PHDR table and find the section of PT_LOAD type that contains the address of the entrypoint method
  phdr = (Elf32_Phdr*)((char*)ehdr + ehdr->e_phoff); 
  //Changed ELF32_Phdr to Elf32_Phdr also added semicolon
  int SegmentNumber = ehdr->e_phnum; //number of program headers
  Elf32_Addr EntryAddress = ehdr->e_entry; //entry point address
  unsigned short phentsize = ehdr->e_phentsize;
  //size of each program header
  //loop through each heade
  
  //The below is for signal handler
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  //Fills the struct with zero
  //Good habit when dealing with signal handlers beacuse of page fault(I Guess?)
  sa.sa_sigaction = crash_handler;
  //Replace the default handler with the one we created
  sa.sa_flags = SA_SIGINFO;
  //tells the OS to provide siginfo_t so that we can get fault address 
  //this installs our handler for "SIGSEGV"
  if(sigaction(SIGSEGV, &sa, NULL) == -1) {
    perror("sigaction failed");
    close(fd);
    free(ELF_Data);
    return;
  }

  //(Aryan)Overwrite the main to _start
  printf("Virtual entry point\n");
  int (*_start)() = (int(*)())EntryAddress;
  int result = _start();
  
  printf("user _start return value= %d\n", result);
}

// int main(int argc, char **argv){
//   if(argc < 2){
//     printf("Missing ELF file");
//     return 1;
//   }
//   load_and_run_elf(argv);
//   loader_cleanup();
// }


//The Elf32_Ehdr and the Elf32_Phdr are in the elf.h header file
//Added semicolons