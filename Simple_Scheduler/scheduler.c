#include "header.h"

#define MAX 1000

struct Process {
    pid_t PID;
    int ArrivalTime;
    int NumWaitSlices;
    int NumCompletionSlices;
    int Completed;
    int IsRunning;
};

//Ready queue
pid_t ReadyQueue[MAX];
int Front = 0;
int Rear = -1;
int Count = 0;

// Checks if the queue is full
int IsFull() {
    return Count == MAX;
}

// Checks if the queue is empty
int IsEmpty() {
    return Count == 0;
}

// Function to add a PID to the ready queue
void Enqueue(pid_t InputPID) {
    if (IsFull()) {
        fprintf(stderr, "Error: Ready Queue is full.\n");
        return;
    }
    Rear = (Rear + 1) % MAX;
    ReadyQueue[Rear] = InputPID;
    Count++;
    printf("Enqueued PID %d -> ReadyQueue Count: %d\n", InputPID, Count);
}

// Function to remove a PID from the ready queue
pid_t Dequeue() {
    if (IsEmpty()) {
        return -1;
    }
    pid_t PID = ReadyQueue[Front];
    Front = (Front + 1) % MAX;
    Count--;
    printf("[Dequeued PID %d <- ReadyQueue Count: %d\n", PID, Count);
    return PID;
}


void PrintHistory(struct Process * ProcessTable, int TotalProcess, int NCPU, int TotalProcesses) {
    printf("\n+++___Final Job Summary___+++\n");
    for (int i = 0; i < TotalProcess; i++) {
        struct Process Stats = ProcessTable[i];
        int CompletitionTime = Stats.NumCompletionSlices - Stats.ArrivalTime;
        if (CompletitionTime < 1 && Stats.Completed) {
            CompletitionTime = 1;
        }
        int WaitingSlices;
        if(NCPU>TotalProcesses){
            WaitingSlices = 0;
        }
        else{
            WaitingSlices = Stats.NumWaitSlices;
        }
        printf("Job PID: %-5d | Completion Time: %-3d slices | Wait Time: %-3d slices\n", Stats.PID, CompletitionTime, WaitingSlices);
    }
    printf("Scheduler shutdown complete.\n");
}


int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <NCPU> <TimeQuanta> <ReadPipe>\n", argv[0]);
        return 1;
    }
    int NCPU = atoi(argv[1]);
    int TimeQuanta = atoi(argv[2]);
    int ReadPipe = atoi(argv[3]);

    // Make the read pipe non-blocking
    fcntl(ReadPipe, F_SETFL, O_NONBLOCK);

    struct Process ProcessTable[MAX];
    int TotalProcess = 0;

    pid_t *RunningProcesses = (pid_t*)malloc(NCPU*sizeof(pid_t));
    if (RunningProcesses == NULL) {
        perror("Memory allocation for RunningProcesses failed");
        return 1;
    }
    for(int i=0;i<NCPU;i++){
        RunningProcesses[i]=0;
    }
    
    int CurrentTimeSlice = 0;
    int ShellTerminatedFlag = 0; // Flag for closed shell pipe

    printf("Scheduler started with %d CPU cores, TimeQuanta=%dms\n", NCPU, TimeQuanta);

    while (1) {
        //Check for the messages from the shell that is the PID of the process that would be run
        pid_t InputPID;
        ssize_t BytesRead;
        while ((BytesRead = read(ReadPipe, &InputPID, sizeof(pid_t))) > 0) {
            if (InputPID > 0) { // A positive PID means a new job
                ProcessTable[TotalProcess].PID = InputPID;
                ProcessTable[TotalProcess].ArrivalTime = CurrentTimeSlice;
                ProcessTable[TotalProcess].NumWaitSlices = 0;
                ProcessTable[TotalProcess].Completed = 0;
                ProcessTable[TotalProcess].NumCompletionSlices = 0;
                ProcessTable[TotalProcess].IsRunning = 0;
                TotalProcess++;
                printf("New process received: PID %d (arrived at slice %d)\n", InputPID, CurrentTimeSlice);
                Enqueue(InputPID);
            } 
            else if (InputPID < 0) { // A negative PID means a job terminated
                pid_t terminated_pid = -InputPID;
                for (int j = 0; j < TotalProcess; j++) {
                    if (ProcessTable[j].PID == terminated_pid) {
                        ProcessTable[j].Completed = 1;
                        ProcessTable[j].NumCompletionSlices = CurrentTimeSlice;
                        printf("[LOG] Process terminated: PID %d (completed at slice %d)\n", terminated_pid, CurrentTimeSlice);
                        break;
                    }
                }
            }
        }
        if (BytesRead == 0 && !ShellTerminatedFlag) { //If it is not reading anymore bytes than that means that the process is completed
            ShellTerminatedFlag = 1;
            printf("Shell Complete.\n");
        }

        //End of time slice
        //In the end of time slice, remove all the process as new processes are to be assigned to the CPU
        for (int i = 0; i < NCPU; i++) {
            pid_t CurrentPID = RunningProcesses[i];
            if (CurrentPID != 0) {
                if (kill(CurrentPID, SIGSTOP) == 0) {
                    printf("CPU[%d]: Stopped PID %d -> Re-enqueueing for next round\n", i, CurrentPID);
                    Enqueue(CurrentPID);
                } 
                else {
                    if (errno == ESRCH) {
                        printf("CPU[%d]: PID %d already exited, skipping requeue\n", i, CurrentPID);
                    } 
                    else {
                        printf("Scheduler: Failed to SIGSTOP running process");
                    }
                    //If the error number is ESRCH than that would mean that there is no such process it the process had already terminate
                }
                RunningProcesses[i] = 0;
            }
        }

        // --- 3. Update wait times for all processes in the ready queue ---
        for (int i = 0; i < Count; i++) {
            pid_t CurrentPid = ReadyQueue[(Front + i) % MAX];
            for (int j = 0; j < TotalProcess; j++) {
                if (ProcessTable[j].PID == CurrentPid) {
                    if(ProcessTable[j].Completed==0){
                    ProcessTable[j].NumWaitSlices++;}
                    break;
                }
            }
        }

        //Dispatch processes to idle CPUs
        for (int i = 0; i < NCPU; i++) {
            if (RunningProcesses[i] == 0 && !IsEmpty()) {
                pid_t PIDRun = Dequeue();
                int is_completed = 0;
                for (int j = 0; j < TotalProcess; j++) {
                    if (ProcessTable[j].PID == PIDRun && ProcessTable[j].Completed) {
                        is_completed = 1;
                        break;
                    }
                }
                if (!is_completed) {
                    RunningProcesses[i] = PIDRun;
                    printf("CPU[%d]: Running PID %d\n", i, PIDRun);
                    kill(PIDRun, SIGCONT);
                } 
                else {
                    printf("PID %d already completed, skipping.\n", PIDRun);
                    i--; // recheck same core
                }
            }
        }

        //Scheduler Shutdown Condition
        //The scheduler shutsdown if there are no more active remaining processes and the shell has terminated
        int ActiveProcess = Count;
        for (int i = 0; i < NCPU; i++) {
            if (RunningProcesses[i] != 0) {
                ActiveProcess++;
            }
        }
        if (ShellTerminatedFlag && ActiveProcess == 0) {
            printf("No active jobs remaining, Exiting scheduler.\n");
            break;
        }

        //Wiat for each time quantqa
        usleep(TimeQuanta * 1000);
        CurrentTimeSlice++;
    }


    PrintHistory(ProcessTable, TotalProcess, NCPU, TotalProcess);

    
    free(RunningProcesses);
    close(ReadPipe);
    return 0;
}