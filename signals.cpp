#include <iostream>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include "signals.h"
#include "Commands.h"



using namespace std;

void ctrlZHandler(int sig_num){
	// TODO: Add your implementation
    std::cout << "smash: got ctrl-Z" << std::endl;
    SmallShell& smash = SmallShell::getInstance();
    JobsList::JobEntry* job = smash.jobs.popFg();
    if(!job){
      return;
    }
    kill(job->pid*(-1), SIGSTOP);
    std::cout << "smash: process " << job->pid << " was stopped" << std::endl;
}

void ctrlCHandler(int sig_num){
    std::cout << "\nsmash: got ctrl-C" << std::endl;
    SmallShell& smash = SmallShell::getInstance();
    JobsList::JobEntry* job = smash.jobs.popFg();
    if(!job){
      return;
    }
    assert(sig_num == SIGINT);
    kill(job->pid*(-1), sig_num);
    std::cout << "smash: process " << job->pid << " was killed" << std::endl;
}

void alarmHandler(int sig_num){
  // TODO: Add your implementation
}

