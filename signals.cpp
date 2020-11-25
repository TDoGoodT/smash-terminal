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
    if(smash.jobs.fg_job) smash.jobs.switchOff(smash.jobs.fg_job);
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

