#include <time.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

// rough timing info of child processes https://stackoverflow.com/questions/52572005/time-the-duration-of-a-program-called-by-execv

#define NUMPROCS 10

typedef struct {
  clock_t start;
  clock_t end;
} diff_t;

void handle_child_dead(int sig, siginfo_t* info, void* ucontext);
void register_handler();
void modify_argv(char** argv, int idx);
void print_time(int i, diff_t* diff);

// globals // structs for maintaining information 
pid_t pids[NUMPROCS]; // do NUMPROCS tests
diff_t diffs[NUMPROCS];


int main(int argc, char* argv[]) {

  // register the handler 
  register_handler();

  /**
   * max iters
   * threshold
   * display frequency
   * seed for rand
   * rows
   * cols
   */
  char* pargv[6] = {
    "100",
    "50",
    "0",
    "1",
    "",
    ""
  };

  // create the processes
  for(int i = 0; i < NUMPROCS; i++) {
    diff_t diff = {.start=clock()};
    diffs[i] = diff;
    pids[i] = fork();  
    if(pids[i] == 0) {
      // child proc
      modify_argv(pargv, i);
      execvp("./array", pargv);
    } else {
      // parent proc
      // do nothing
      printf("Created child %d\n", pids[i]);
    }
  }

  // wait for the processes
  int n = NUMPROCS;
  int status;
  while(n--) {
    waitpid(pids[n], &status, 0); 
    if(status != 0) {
      printf("%d: exited with status %d\n", pids[n], status);
    }
  } 

  // report the timing information
  for(int i = 0; i < NUMPROCS; i++) {
    print_time(i, &diffs[i]);
  }

  return 0;
}

void handle_child_dead(int sig, siginfo_t* info, void* ucontext) {
  int pid = info->si_pid;
  int ind = 0;
  while(ind < NUMPROCS && pids[ind] != pid) ind++;
   
  diffs[ind].end = clock();
  printf("%d: child terminated\n", pid);
  print_time(-2, &diffs[ind]);
}

void register_handler(){
  struct sigaction act;

  act.sa_flags = SA_SIGINFO | SA_NOCLDWAIT | SA_RESTART;
  act.sa_sigaction = &handle_child_dead;
  if (sigaction(SIGCHLD, &act, NULL) == -1) {
    perror("sigaction");
    exit(1);
  }
}

void modify_argv(char** argv, int idx) {
  /** 
   * 0 max iters
   * 1 threshold
   * 2 display frequency
   * 3 seed for rand
   * 4 rows
   * 5 cols
   * */
  sprintf(argv[4], "%d", idx+2);
  sprintf(argv[5], "%d", idx+2);
}

void print_time(int i, diff_t* diff) {
  clock_t clock_diff = diff->end - diff->start;
  int millis = clock_diff * 1000 / CLOCKS_PER_SEC;
  // i + 2 comes from the func used to determine the size in modify_argv
  printf("%dx%d - Time: %d sec %d millisec\n", i+2, i+2, millis/1000, millis%1000);   
}

