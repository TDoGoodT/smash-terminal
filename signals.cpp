#include <iostream>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include "signals.h"
#include "Commands.h"



using namespace std;

void ctrlZHandler(int sig_num)
{
  std::cout << "smash: got ctrl-Z" << std::endl;
  SmallShell& smash = SmallShell::getInstance();
  //reset alarm?
  JobsList::JobEntry* job = smash.jobs.popFg();
  if(!job){
    return;
  }
  kill(job->pid*(-1), SIGTSTP);
  std::cout << "smash: process " << job->pid << " was stopped" << std::endl;
}

void ctrlCHandler(int sig_num)
{
  std::cout << "smash: got ctrl-C" << std::endl;
  SmallShell& smash = SmallShell::getInstance();
  //reset alarm?
  JobsList::JobEntry* job = smash.jobs.popFg();
  if(!job){
    return;
  }
  kill(job->pid*(-1), SIGKILL);
  std::cout << "smash: process " << job->pid << " was killed" << std::endl;
}

void alarmHandler(int sig_num)
{
  SmallShell& smash = SmallShell::getInstance();
  auto cmd_to_kill = smash.timed_cmds.begin();
  if(cmd_to_kill != smash.timed_cmds.end()){
    if(waitpid(cmd_to_kill->second->pid*(-1), nullptr, WNOHANG) == 0){
        assert(cmd_to_kill->first == time(nullptr));
        std::cout << "smash: got an alarm" << std::endl;
        kill(cmd_to_kill->second->pid*(-1),SIGKILL);
        std::cout << cmd_to_kill->second->orig_cmd_line << " timed out!" << std::endl;
    }
    smash.timed_cmds.erase(cmd_to_kill);
  }
  auto next_cmd_to_kill = smash.timed_cmds.begin();
  if(next_cmd_to_kill != smash.timed_cmds.end()){
    alarm(difftime(cmd_to_kill->first,time(nullptr)));
  }else {
    alarm(0);
  }

}

