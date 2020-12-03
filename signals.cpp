#include <iostream>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include "signals.h"
#include "Commands.h"



using namespace std;


void ctrlZHandler(int sig_num)
{
  SmallShell& smash = SmallShell::getInstance();
  smash.closeFiles();
  JobsList::JobEntry* job = smash.jobs.popFg();
  if(!job){
    std::cout << "smash: got ctrl-Z" << std::endl;
    return;
  }
  std::cout << "smash: got ctrl-Z" << std::endl;
  if(kill(job->pid*(-1), SIGTSTP) < 0) perror("smash error: kill failed");
  else std::cout << "smash: process " << job->pid << " was stopped" << std::endl;
}

void ctrlCHandler(int sig_num)
{
  SmallShell& smash = SmallShell::getInstance();
  smash.closeFiles();
  std::cout << "smash: got ctrl-C" << std::endl;
  JobsList::JobEntry* job = smash.jobs.popFg();
  if(!job){
    return;
  }
  if(kill(job->pid*(-1), SIGKILL) < 0) perror("smash error: kill failed");
  else std::cout << "smash: process " << job->pid << " was killed" << std::endl;
}

void alarmHandler(int sig_num)
{
  SmallShell& smash = SmallShell::getInstance();
  smash.closeFiles();
  smash.jobs.removeFinishedJobs();
  std::cout << "smash: got an alarm" << std::endl;
  auto cmd_to_kill = smash.timed_cmds.begin();
  if(cmd_to_kill != smash.timed_cmds.end()){
    if(cmd_to_kill->second->pid != smash.pid && kill(cmd_to_kill->second->pid, 0) == 0){
      int w = waitpid(cmd_to_kill->second->pid,nullptr,WNOHANG);
      if(w < 0) perror("smash error: waitpid failed");
      else if(w == 0){
        if(kill(cmd_to_kill->second->pid*(-1),SIGKILL) < 0) perror("smash error: kill failed");
        else std::cout << "smash: " << cmd_to_kill->second->orig_cmd_line << " timed out!" << std::endl;
      }
    }
    smash.timed_cmds.erase(cmd_to_kill);
  }
  auto next_cmd_to_kill = smash.timed_cmds.begin();
  if(next_cmd_to_kill != smash.timed_cmds.end()){
    alarm((unsigned int)difftime(next_cmd_to_kill->first,time(nullptr)));
  }else {
    alarm(0);
  }

}

