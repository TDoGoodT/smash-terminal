#include <iostream>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include "signals.h"
#include "Commands.h"



using namespace std;

void ctrlZHandler(int sig_num)
{
	// TODO: Add your implementation
    std::cout << "\nsmash: got ctrl-Z in ";
    SmallShell& smash = SmallShell::getInstance();
    //smash.jobs.switchOff(smash.jobs.getLastJob());
    JobsList::JobEntry* job = smash.jobs.popFg();
    job->execution_state = Waiting;
    smash.jobs.insertNewJob(job);
    kill(job->pid, sig_num);
    std::cout << "smash: process " << job->pid << " was stopped with signal num " << sig_num << std::endl;
//    kill(smash.jobs.getLastJob()->pid*(-1), SIGSTOP);
}

void ctrlCHandler(int sig_num)
{
  // TODO: Add your implementation
    std::cout << "smash: got ctrl-C in " << getpid();
    //kill(getpid(),sig_num);
}

void alarmHandler(int sig_num)
{
  // TODO: Add your implementation
}

