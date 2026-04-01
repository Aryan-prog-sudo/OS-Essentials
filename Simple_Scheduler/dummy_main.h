#include<signal.h>
#include<unistd.h>
int dummy_main(int argc,char **argv);

int main(int argc,char **argv){
    raise(SIGSTOP);
    int ret =dummy_main(argc,argv);
    return ret;
}

#define main dummy_main
//in the processing stage the macros are replace this command replaces the main keyword with dummy_main
//allows schedler control any user program 