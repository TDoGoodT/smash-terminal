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
    std::cout << "smash: got ctrl-Z in " << getpid();
    SmallShell& smash = SmallShell::getInstance();
    JobsList::JobEntry* last_job = smash.jobs.getLastJob(NULL);
    kill(last_job->pid,SIGSTOP);
}

void ctrlCHandler(int sig_num)
{
  // TODO: Add your implementation
    std::cout << "smash: got ctrl-C in " << getpid();
    kill(getpid(),SIGKILL);
}

void alarmHandler(int sig_num)
{
  // TODO: Add your implementation
}

