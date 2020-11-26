#include <dirent.h>
#include <algorithm>
#include <unistd.h>
#include <string>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <iomanip>
#include <signal.h>
#include <string>
#include <stdlib.h>



#include "Commands.h"

using namespace std;
const std::string WHITESPACE = " \n\r\t\f\v";

std::string SmallShell::oldp = "";
#if 0
#define FUNC_ENTRY()  \
  cerr << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cerr << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

#define DEBUG_PRINT cerr << "DEBUG: "

#define EXEC(path, arg) \
  execvp((path), (arg));

string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}
const vector<string> explode(const string& s, const char c)
{
    string buff{""};
    vector<string> v;

    for(auto n:s)
    {
        if(n != c) buff+=n; else
        if(n == c && buff != "") { v.push_back(buff); buff = ""; }
    }
    if(buff != "") v.push_back(buff);

    return v;
}

int _parseCommandLine(const char* cmd_line, char** args) {
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args[i] = (char*)malloc(s.length()+1);
    memset(args[i], 0, s.length()+1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}

bool _isBackgroundCommand(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == string::npos) {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}


static const string _getCurrDir()
{
    char buff[BUFSIZ];
    getcwd(buff, BUFSIZ);
    return string(buff);
}

// TODO: Add your implementation for classes in Commands.h

SmallShell::SmallShell():
    name("smash"), jobs(){}

SmallShell::~SmallShell() {
// TODO: add your implementation
}
const string SmallShell::getName()
{
    return name;
}
const string SmallShell::setName(const string new_name)
{
    string old_name(name);
    name = new_name;
    return old_name;
}
/*
void SmallShell::addJob(Command* cmd,  pid_t pid, bool isStopped)
{
    this->jobs.addJob(cmd,pid);
}*/

Command::Command(const char* cmd_line) {
// TODO: add your implementation
    this->cmd_line = string(cmd_line);
    char new_cmd_line[this->cmd_line.length()+1];

    strcpy(new_cmd_line, cmd_line);
    if(_isBackgroundCommand(cmd_line))
    {
        _removeBackgroundSign(new_cmd_line);
        type = Background;
    } else type = Foreground;
    this->cmd_line = string(new_cmd_line);
    _parseCommandLine(new_cmd_line, args);
}



void BuiltInCommand::execute()
{
// TODO: add your implementation
}

ExternalCommand::ExternalCommand(const char* cmd_line, SmallShell* smash_p):
        Command(cmd_line),
        smash_p(smash_p)
{
    _parseCommandLine(this->cmd_line.c_str(), args);
    this->cmd_line = string("bash -c \" ") + (string(cmd_line)) + string(" \"");
    //cout << this->cmd_line << "\n";
    //strcpy(args[2], this->cmd_line.c_str());
}
void ExternalCommand::execute()
{
// TODO: add your implementation
    if(!args[0] || strcmp(args[0],"") == 0) return;
    //cout << "1: " << args[0] << " 2: " << args[1] << " 3: " << args[2] << std::endl;
    //cout << args[0] << std::endl;


    pid_t c_pid = fork();
    if(c_pid > 0)
    {
        int option = type == Foreground ? 0 : WNOHANG;
        if(!waitpid(c_pid, return_status, option))
        {
            assert(smash_p->jobs.fg_job == nullptr);
            JobsList::JobEntry* new_job = new JobsList::JobEntry(0, c_pid, Running, this);
            smash_p->jobs.fg_job = new_job;
            int wstatus = -1;
            do {
                int w = waitpid(c_pid, &wstatus,  WNOHANG | WUNTRACED);
                if (w == -1) {
                    perror("waitpid");
                    exit(EXIT_FAILURE);
                }

                if (WIFEXITED(wstatus)) {
                    printf("exited, status=%d\n", WEXITSTATUS(wstatus));
                } else if (WIFSIGNALED(wstatus)) {
                    printf("pid %d killed by signal %d\n", WTERMSIG(wstatus));
                } else if (WIFSTOPPED(wstatus)) {
                    printf("pid %d stopped by signal %d\n", c_pid, WSTOPSIG(wstatus));
                    break;
                } else if (WIFCONTINUED(wstatus)) {
                    printf("continued\n");
                }
            } while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));
            //smash_p->jobs.fg_job = nullptr;
        }
        else
        {
            if(!waitpid(c_pid, NULL, WNOHANG)) //if the new proc didnt finshed
                    smash_p->jobs.addJob(this,c_pid);
        }
    }
    else
    {
        setpgrp();
        char cmd[COMMAND_ARGS_MAX_LENGTH];
        strcpy(cmd, cmd_line.c_str());
        char bash[] = "bash";
        char flags[] = "-c";
        char * argv[] = {bash, flags, cmd, NULL};
        //cout << cmd << '\n';
        EXEC("/bin/bash", argv);
        assert(0);
    }
}

void GetCurrDirCommand::execute(){
// TODO: test
    cout << _getCurrDir() << "\n";
}



void GetDirContentCommand::execute()
{
// TODO: add your implementation
    struct dirent **namelist;
    int n = scandir(".", &namelist, 0, alphasort);
    if (n < 0) perror("smash error: scandir failed");
    else
    {
        for (int i = 0; i < n; i++)
        {
            cout << namelist[i]->d_name << "\n";
            free(namelist[i]);
        }
    }
    free(namelist);


}


void ChangePromptCommand::execute(){
// TODO: test
    if(!args[1]) smash_p->setName(); // no args -> set name to be "smash"
    else smash_p->setName(args[1]); // else -> set shell name to be the 1st argument
}

void ShowPidCommand::execute(){

    cout << "smash pid is " << getpid() << "\n";
}
ChangeDirCommand::ChangeDirCommand(const char *cmd_line, std::string plastPwd):BuiltInCommand(cmd_line),old(plastPwd){}
void ChangeDirCommand::execute(){
    std::string oldpath = get_current_dir_name();
    const vector<string> s=explode(cmd_line,' ');
    if(args[2]){                                                    // Too many args
        perror("smash error: cd: too many arguments");
    }else{                                                          //Less then 2 arguments
        if(args[1]){                                                //Exacly 2 arguments
            char p[s[1].length()+1];
            for (int i = 0; i < sizeof(p); i++) {
                p[i] = s[1][i];
            }
            p[s[1].length()]=0;
            if(s[1]=="-"){
                if(old==""){
                    std::cerr<<"smash error: cd: OLDPWD not set"<<std::endl;
                }else{
                    char p[old.length()+1];
                    for (int i = 0; i < old.length()+1; i++) {
                        p[i] = old[i];
                    }
                    long size = pathconf(".",_PC_PATH_MAX);
                    char* new_arr;
                    char* tmp;
                    if ((new_arr = (char *)malloc((size_t)size)) != NULL) {
                        tmp = getcwd(new_arr, (size_t) size);
                        if(!tmp){
                            perror("smash error: getcwd failed");
                            return;
                        }
                    }
                    if(chdir(p)!=0){
                        perror("smash error: chdir failed");
                        return;
                    }
                    SmallShell::oldp=tmp;
                    free(new_arr);
                }
            }else{
                long size = pathconf(".", _PC_PATH_MAX);
                char* new_arr;
                char* tmp;
                if ((new_arr = (char *)malloc((size_t)size)) != NULL) {
                    tmp = getcwd(new_arr, (size_t) size);
                    if(!tmp){
                        perror("smash error: getcwd failed");
                        return;
                    }
                }
                if(chdir(p) != 0){
                    perror("smash error: chdir failed");
                    return;
                }
                SmallShell::oldp = tmp;
                free(new_arr);
            }
        } else {
            return;
        }
    }


}

void JobsCommand::execute()
{
// TODO: add your implementation
    jobs->printJobsList();
}

bool is_number(const std::string& s)
{
    return !s.empty() && std::find_if(s.begin(),
        s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();
}

void ForegroundCommand::execute(){
// TODO: test
    if(args[1])
        if(is_number(string(args[1]))) job_id = std::stoi(args[1]);
        else
        {
            perror("smash error: fg: invalid arguments");
            return;
        }
    JobsList::JobEntry* last_stopped = (job_id > 0) ? jobs->getJobById(job_id): jobs->getLastStoppedJob(&job_id);
    if(!last_stopped)
    {
        string error_s;
        if(!job_id)
            error_s = string("smash error: fg: jobs list is empty");
        else
            error_s = string("smash error: fg: job-id ",job_id) + string( " does not exist");
        perror(error_s.c_str());
        return;
    }
    if(*std::find(jobs->waiting_queue.begin(), jobs->waiting_queue.end(), job))
    {
        jobs->switchJobOn(job, true);
    }
    else
    {
        cout << cmd_line.c_str() << " : " << last_stopped->pid << "\n";
        waitpid(last_stopped->pid,NULL,0);
    }
    cout << cmd_line.c_str() << " : " << job->pid << "\n";
    //jobs->removeJobById(job_id);

     while(!waitpid(job->pid,NULL, WNOHANG) && jobs->fg_job) {}
    //jobs->removeJob(job);
}

void QuitCommand::execute()
{
    return;
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
//Parse arguments
    char* args[COMMAND_MAX_ARGS];
    _parseCommandLine(cmd_line, args);
//Get the first word in the command
    string cmd_s(args[0]);
    if (cmd_s == "pwd") {
        return new GetCurrDirCommand(cmd_line);
    }

    else if (cmd_s == "chprompt") {
        return new ChangePromptCommand(cmd_line, this);
    }
    else if (cmd_s == "ls") {
        return new GetDirContentCommand(cmd_line);
    }
    else if (cmd_s == "showpid") {
        return new ShowPidCommand(cmd_line);
    }
    else if (cmd_s == "cd") {
        return new ChangeDirCommand(cmd_line, oldp);
    }
    else if (cmd_s == "jobs") {
        return new JobsCommand(cmd_line, &jobs);
    }
    else if (cmd_s == "fg") {
        return new ForegroundCommand(cmd_line, &jobs);
    }
    else if (cmd_s == "quit") {
        return new QuitCommand(cmd_line, &jobs);
    }

    else {
    return new ExternalCommand(cmd_line, this);
    }

    //return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
    // TODO: Add your implementation here
    // for example:
    Command* cmd = CreateCommand(cmd_line);
    jobs.removeFinishedJobs();
    cmd->execute();
//    delete cmd;

    // Please note that you must fork smash process for some commands (e.g., external commands....)
}

