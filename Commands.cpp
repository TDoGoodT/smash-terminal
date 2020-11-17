#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>

#include "Commands.h"

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

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

bool _isBackgroundComamnd(const char* cmd_line) {
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

SmallShell::SmallShell() {
// TODO: add your implementation
}

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

Command::Command(const char* cmd_line) {
// TODO: add your implementation
    this->cmd_line = string(cmd_line);
    _parseCommandLine(cmd_line, args);
}

Command::~Command() {
// TODO: add your implementation
}

BuiltInCommand::BuiltInCommand(const char* cmd_line):
    Command(cmd_line)
{
// TODO: add your implementation
}

BuiltInCommand::~BuiltInCommand()
{
// TODO: add your implementation
}

void BuiltInCommand::execute()
{
// TODO: add your implementation
}


ExternalCommand::ExternalCommand(const char* cmd_line):
    Command(cmd_line)
{
// TODO: add your implementation
}

ExternalCommand::~ExternalCommand()
{
// TODO: add your implementation
}


void ExternalCommand::execute()
{
// TODO: add your implementation
    int *return_status = NULL;
    pid_t c_pid;
    c_pid = fork();
    if(c_pid > 0)
    {
        cout << "start\n";
        waitpid(c_pid, return_status,0);
        cout << "end\n";
    }
    else
    {
        char cmd[COMMAND_MAX_ARGS*(2+COMMAND_ARGS_MAX_LENGTH)];
        strcpy(cmd, cmd_line.c_str());
        cout << cmd << "\n";
        char *argv[] ={"bash", "-c", cmd, NULL};
        execv("/bin/bash",argv);
    }
}

GetCurrDirCommand::GetCurrDirCommand(const char* cmd_line):
    BuiltInCommand(cmd_line)
{
// TODO: add your implementation
}

GetCurrDirCommand::~GetCurrDirCommand()
{
// TODO: add your implementation
}

void GetCurrDirCommand::execute(){
// TODO: test
    cout << _getCurrDir() << "\n";
}


GetDirContentCommand::GetDirContentCommand(const char* cmd_line):
    BuiltInCommand(cmd_line)
{
// TODO: add your implementation
}

GetDirContentCommand::~GetDirContentCommand()
{
// TODO: add your implementation
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


ChangePromptCommand::ChangePromptCommand(const char* cmd_line, SmallShell* smash_p):
    BuiltInCommand(cmd_line),
    smash_p(smash_p){
// TODO: add your implementation
}

ChangePromptCommand::~ChangePromptCommand(){
// TODO: add your implementation
}
void ChangePromptCommand::execute(){
// TODO: test
    if(!args[1]) smash_p->setName();
    else smash_p->setName(args[1]);
}


/*
JobsList::JobsList(){}
JobsList::~JobsList(){}
void JobsList::addJob(Command* cmd, bool isStopped = false){}
void JobsList::printJobsList(){}
void JobsList::killAllJobs(){}
void JobsList::removeFinishedJobs(){}
JobsList::JobEntry * JobsList::getJobById(int jobId){}
void JobsList::removeJobById(int jobId){}
JobsList::JobEntry * JobsList::getLastJob(int* lastJobId){}
JobsList::JobEntry *JobsList::getLastStoppedJob(int *jobId){}
*/

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

    else {
    return new ExternalCommand(cmd_line);
    }

    //return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
    // TODO: Add your implementation here
    // for example:
    Command* cmd = CreateCommand(cmd_line);

    cmd->execute();
//    delete cmd;

    // Please note that you must fork smash process for some commands (e.g., external commands....)
}

