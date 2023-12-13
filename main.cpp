#include <iostream>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <csignal>
#include <cstdlib>
#include <vector>
using namespace std;

#define N 10  // process 개수
#define MSG_KEY_P 1234  // p->c message queue의 key
#define MSG_KEY_C 1235  // c->p message queue의 key
#define TQ 4  // time quantum
#define TT 1  // time tick

void child_process(int i, int cpuBurst);
void processSwitching();
void processTerminating();
void schedulerTerminating();
void timerHandler(int signum);
void printReadyQueue();
void determiningIO(int cpuBurst, int &start, int &burst);

// IPC message
struct Message {
  long type;
  int flag;
};
// PCB
struct PCB {
  int num;
  pid_t pid;
  int cpuBurst;
  int ioBurst;
  int ioStart;
};

vector<PCB*> readyQueue;
int idx = 1;
int curTQ = 0;
int msg_queue_p;
int msg_queue_c;


// child process
void child_process(int i, int cpuBurst) {
  Message parentMsg, childMsg;

  // message queue
  int msg_queue_p = msgget((key_t)MSG_KEY_P, 0666|IPC_CREAT);
  if (msg_queue_p==-1) {
    perror("msgget");
    exit(EXIT_FAILURE);
  }
  int msg_queue_c = msgget((key_t)MSG_KEY_C, 0666|IPC_CREAT);
  if (msg_queue_c==-1) {
    perror("msgget");
    exit(EXIT_FAILURE);
  }

  sleep(2);

  // recieve message
  while(true) {
    if (msgrcv(msg_queue_p, &parentMsg, sizeof(Message)-sizeof(long), i, IPC_NOWAIT)==-1) continue;
    if (cpuBurst>0) cpuBurst--;
    if (cpuBurst==0) {
      childMsg.type = i; childMsg.flag = 1;
      if (msgsnd(msg_queue_c, &childMsg, sizeof(Message)-sizeof(long), 0)==-1) {
        perror("msgsnd");
        exit(EXIT_FAILURE);
      }
      // cout << "  [CHILD] #" << i << " " << getpid() << " ended" << endl;
      exit(0);
    }
    // cout << "remained CPU " << i << ": " << cpuBurst << endl;
  }
}


void processSwitching() {
  if (readyQueue.size()==1) return;
  curTQ = 0;
  // cout << "[ switch: " << readyQueue[0]->num;
  readyQueue.push_back(readyQueue[0]);
  readyQueue.erase(readyQueue.begin());
  // cout << " to " << readyQueue[0]->num << " ]" << endl;
  idx = readyQueue[0]->num;
}

void processTerminating() {
  if (readyQueue.size()==1) {
    readyQueue.erase(readyQueue.begin());
    schedulerTerminating();
  }
  curTQ = 0;
  // cout << "[ terminate: " << readyQueue[0]->num;
  readyQueue.erase(readyQueue.begin());
  // cout << ", switch to " << readyQueue[0]->num << " ]" << endl;
  idx = readyQueue[0]->num;
}

void schedulerTerminating() {
  printReadyQueue();
  cout << "[PARENT] " << getpid() << " ended" << endl;
  exit(0);
}

void printReadyQueue() {
  cout << "Ready Queue: [ ";
  for (auto process : readyQueue) {
    cout << process->num << "(" << process->pid << "): " << process->cpuBurst << "s";
    if (*(readyQueue.end())!=process) cout << ", ";
  }
  cout << " ]" << endl;
}

void determiningIO(int cpuBurst, int &start, int &burst) {
  srand((unsigned int)time(NULL));
  bool flag = rand()%2;
  if (flag) {
    start = (rand()%(cpuBurst-1)) + 1;
    burst = (rand()%5) + 1;
  }
  else { start = 0; burst = 0; }
  return;
}



// SIGALRM handler
void timerHandler(int signum) {
  Message parentMsg, childMsg;
  parentMsg.type = idx; parentMsg.flag = 0;

  if (msgrcv(msg_queue_c, &childMsg, sizeof(Message)-sizeof(long), 0, IPC_NOWAIT)!=-1) {
    processTerminating();
    parentMsg.type = idx;
  }
  else if (curTQ==TQ) {
    processSwitching();
    parentMsg.type = idx;
  }

  readyQueue[0]->cpuBurst--;
  printReadyQueue();

  if (msgsnd(msg_queue_p, &parentMsg, sizeof(Message)-sizeof(long), 0)==-1) {
    perror("msgsnd");
    exit(EXIT_FAILURE);
  }

  curTQ++;
}


int main() {
  pid_t ppid=getpid();
  for (int i=0; i<N; i++) readyQueue.push_back(new PCB());

  // create child processes
  for (int i=0; i<N; i++) {

    pid_t pid = fork();

    readyQueue[i]->num = i+1;
    readyQueue[i]->pid = pid;

    // determining CPU burst time
    srand((unsigned int)time(NULL)*i);
    int n = (rand()%10) + 1;
    readyQueue[i]->cpuBurst = n;

    // determining IO burst time
    determiningIO(n, readyQueue[i]->ioStart, readyQueue[i]->ioBurst);

    // error
    if (pid<0) {
      cout << "Error" << endl;
      return -1;
    }
    // parent process
    else if (pid!=0) {
      cout << "[PARENT] created #" << i+1 << " " << readyQueue[i]->num << " -> " << readyQueue[i]->cpuBurst << endl;
    }
    // child process
    else child_process(i+1, readyQueue[i]->cpuBurst);
  }
  cout << endl;

  sleep(1);

  // signal handler
  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_handler = &timerHandler;
  sigaction(SIGALRM, &action, nullptr);

  // timer setting
  struct itimerval timer;
  timer.it_value.tv_sec = TT; timer.it_value.tv_usec = 0;
  timer.it_interval.tv_sec = TT; timer.it_interval.tv_usec = 0;
  if (setitimer(ITIMER_REAL, &timer, nullptr) == -1) {
    perror("setitimer");
  }

  // message queue
  msg_queue_p = msgget((key_t)MSG_KEY_P, 0666|IPC_CREAT);
  if (msg_queue_p==-1) {
    perror("msgget");
  }
  msg_queue_c = msgget((key_t)MSG_KEY_C, 0666|IPC_CREAT);
  if (msg_queue_c==-1) {
    perror("msgget");
  }

  while(true) {}
}