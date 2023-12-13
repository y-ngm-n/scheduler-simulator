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
  int flag;
};

int main() {
  int msg_queue_p = msgget((key_t)1234, 0666|IPC_CREAT);
  if (msg_queue_p==-1) {
    perror("msgget");
    return -1;
  }
  int msg_queue_c = msgget((key_t)1235, 0666|IPC_CREAT);
  if (msg_queue_c==-1) {
    perror("msgget");
    return -1;
  }

  while(true) {
    sleep(1);
    Message m;
    if (msgrcv(msg_queue_p, &m, sizeof(Message)-sizeof(long), 0, IPC_NOWAIT)==-1) {
      perror("msgrcv");
      break;
    }
    cout << m.type << ": " << m.flag << endl;
  }

  while(true) {
    sleep(1);
    Message m;
    if (msgrcv(msg_queue_c, &m, sizeof(Message)-sizeof(long), 0, IPC_NOWAIT)==-1) {
      perror("msgrcv");
      break;
    }
    cout << m.type << ": " << m.flag << endl;
  }
}