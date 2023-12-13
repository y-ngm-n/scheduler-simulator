p, &m, sizeof(Message)-sizeof(long), 0, IPC_NOWAIT)==-1) {
    //   perror("msgrcv");
    //   continue;
    // }