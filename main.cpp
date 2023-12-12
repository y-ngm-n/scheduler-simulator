#include <iostream>
#include <unistd.h>
// #include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <csignal>
using namespace std;

#define N 10  // process 개수
#define MSG_SIZE 1024  // ?
#define MSG_KEY 1234  // message queue의 key
#define TQ 4  // time quantum


// IPC message
struct Message {
  long type;
  int num;
  char buffer[MSG_SIZE];
};

// PCB
struct PCB {
  pid_t pid;
  int cpuBurst;
};

// child process
void child_process(int i) {
  Message m;
  int msg_queue = msgget((key_t)MSG_KEY, 0666|IPC_CREAT);
  if (msg_queue==-1) {
    perror("msgget");
    exit(EXIT_FAILURE);
  }

  sleep(3);

  // int result = msgrcv(msg_queue, &m, sizeof(Message)-sizeof(long), i, 0);
  // cout << i << ": " << m.buffer << endl;
  while(true) {
    int result = msgrcv(msg_queue, &m, sizeof(Message)-sizeof(long), i, IPC_NOWAIT);
    if (result==-1) continue;
    cout << i << ": " << m.buffer << endl;
    break;
  }

  cout << "  [CHILD] #" << i << " " << getpid() << " ended" << endl;
  exit(0);
}


pid_t processes[N];
PCB* readyQueue[N];
int idx = 1;
int msg_queue;


// SIGALRM handler
void timerHandler(int signum) {
  Message m;
  m.type = idx; m.num = idx; strcpy(m.buffer, "Hi");

  if (msgsnd(msg_queue, &m, sizeof(Message)-sizeof(long), 0)==-1) {
    perror("msgsnd");
    exit(EXIT_FAILURE);
  }

  cout << "[PARENT] sended msg " << idx << endl;
  idx = (idx+1) % (N+1);
}


int main() {
  pid_t ppid=getpid();

  // create child processes
  for (int i=0; i<N; i++) {

    pid_t pid = fork();

    // error
    if (pid<0) {
      cout << "Error" << endl;
      return -1;
    }
    // child process
    else if (pid==0) {
      child_process(i+1);
    }
    // parent process
    else {
      processes[i] = pid;
      cout << "[PARENT] created #" << i+1 << " " << processes[i] << endl;
    }
  }
  for (auto pid : processes) cout << pid << " ";
  cout << endl;

  // signal handler
  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_handler = &timerHandler;
  sigaction(SIGALRM, &action, nullptr);

  // timer setting
  struct itimerval timer;
  timer.it_value.tv_sec = TQ; timer.it_value.tv_usec = 0;
  timer.it_interval.tv_sec = TQ; timer.it_interval.tv_usec = 0;
  if (setitimer(ITIMER_REAL, &timer, nullptr) == -1) {
    perror("setitimer");
  }

  // message queue
  msg_queue = msgget((key_t)MSG_KEY, 0666|IPC_CREAT);
  if (msg_queue==-1) {
    perror("msgget");
  }

  for (auto process : processes) {
    waitpid(process, nullptr, 0);
  }

  cout << "[PARENT] " << ppid << " ended" << endl;
  return 0;
}