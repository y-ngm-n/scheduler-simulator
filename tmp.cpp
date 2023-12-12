#include <iostream>
#include <unistd.h>
// #include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <csignal>
using namespace std;

// IPC message
struct Message {
  long type;
  int num;
  char buffer[1024];
};

int main() {
  int msg_que = msgget((key_t)1234, 0666|IPC_CREAT);
  if (msg_que==-1) {
    perror("msgget");
    return -1;
  }

  while(true) {
    sleep(1);
    Message m;
    if (msgrcv(msg_que, &m, sizeof(Message)-sizeof(long), 0, IPC_NOWAIT)==-1) {
      perror("msgrcv");
      continue;
    }
    cout << m.type << ": " << m.buffer << endl;
  }
}