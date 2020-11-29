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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>



#include "Commands.h"

using namespace std;
const string WHITESPACE = " \n\r\t\f\v";

string SmallShell::oldp = "";
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
string _ltrim(const string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == string::npos) ? "" : s.substr(start);
}

string _rtrim(const string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const string& s)
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
    istringstream iss(_trim(string(cmd_line)).c_str());
    for(string s; iss >> s; ) {
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

bool _isRedirectonCommand(const char* cmd_line) {
    const string str(cmd_line);
    return (str.find_first_of(">") != string::npos);
}

bool _isPipeCommand(const char* cmd_line) {
    const string str(cmd_line);
    return (str.find_first_of("|") != string::npos);
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

void _removeFirstOfSign(char* cmd_line, string sign) {
  const string str(cmd_line);
  unsigned int idx = str.find_first_of(sign);
  if (idx == string::npos) {
    return;
  }
  if (cmd_line[idx] != sign[0] && ( sign.length() == 1 || cmd_line[idx+1] != sign[1])) {
    return;
  }
  cmd_line[idx] = ' ';
  if(sign.length() > 1){
      assert(sign.length() == 2);
      cmd_line[idx + 1] = ' ';
  }
}

void _removeLastOfSignAndTrunc(char* cmd_line, string sign) {
  const string str(cmd_line);
  unsigned int idx = str.find_last_of(sign);
  if (idx == string::npos) {
    return;
  }
  if (cmd_line[idx] != sign[0] && ( sign.length() == 1 || cmd_line[idx+1] != sign[1])) {
    return;
  }
  cmd_line[idx] = ' ';
  if(sign.length() > 1){
      assert(sign.length() == 2);
      cmd_line[++idx] = ' ';
  }
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}


static const string _getCurrDir()
{
    return string(get_current_dir_name());
}

bool _isNumber(const string& s){
    return !s.empty() && find_if(s.begin(),
        s.end(), [](unsigned char c) { return !isdigit(c); }) == s.end();
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
BuiltInCommand::BuiltInCommand(const char* cmd_line):
    Command(cmd_line) {
    assert(_isBackgroundCommand(cmd_line) == false);//_removeBackgroundSign(new_cmd_line);
    type = Foreground;
    _parseCommandLine(cmd_line,args);
    this->cmd_line = string(cmd_line);
}
void BuiltInCommand::execute(){
// TODO: add your implementation
}
ExternalCommand::ExternalCommand(const char* cmd_line, SmallShell* smash_p):
        Command(cmd_line),
        smash_p(smash_p){
    if(_isBackgroundCommand(cmd_line)){
        type = Background;
        char new_cmd_line[strlen(cmd_line)];
        strcpy(new_cmd_line, cmd_line);
        _removeBackgroundSign(new_cmd_line);
        run_cmd = string(new_cmd_line);
    }
    else {
        type = Foreground;
        run_cmd = this->cmd_line;
    }
    _parseCommandLine(run_cmd.c_str(),args);
}
void ExternalCommand::execute(){
// TODO: add your implementation
    if(!args[0] || strcmp(args[0],"") == 0) return;
    pid_t c_pid = fork();
    if(c_pid > 0){
        assert(smash_p->jobs.fg_job == nullptr);
        JobsList::JobEntry* new_job;
        if(type == Foreground){
            new_job = new JobsList::JobEntry(0, c_pid, Running, this);
            smash_p->jobs.fg_job = new_job;
            smash_p->jobs.waitForJob(new_job);
        }else{
            smash_p->jobs.addJob(this, c_pid);
        }
    }
    else{
        setpgrp();
        char cmd[COMMAND_ARGS_MAX_LENGTH];
        strcpy(cmd, run_cmd.c_str());
        char bash[] = "/bin/bash";
        char flags[] = "-c";
        char* argv[4];
        argv[0] = bash;
        argv[1] = flags;
        argv[2] = cmd;
        argv[3] = NULL;
        EXEC(*argv, argv);
        assert(0);
    }
}
RedirectionCommand::RedirectionCommand(const char* cmd_line, SmallShell* smash_p):
    Command(cmd_line), append(false), fd(-1), smash_p(smash_p){
        bool is_bg = false;
        char cmd_line_cpy[strlen(cmd_line)];
        strcpy(cmd_line_cpy, cmd_line);
        _removeFirstOfSign(cmd_line_cpy, ">");
        if(_isRedirectonCommand(cmd_line_cpy)){
            append = true;
            _removeFirstOfSign(cmd_line_cpy, ">");
        }
        if(_isBackgroundCommand(cmd_line_cpy)){
            _removeBackgroundSign(cmd_line_cpy);
            is_bg = true;
        }
        int argn = _parseCommandLine(cmd_line_cpy, args);
        out_file_path = args[argn-1];
        string  new_cmd = _rtrim(string(cmd_line_cpy));
        cout << cmd_line_cpy << endl;
        cmd = smash_p->CreateCommand(new_cmd.c_str());
        if(cmd != nullptr) 
            cmd->type = is_bg ? Background : Foreground;
    }
void RedirectionCommand::execute(){
    pid_t c_pid = fork();
    if(c_pid == 0){
        if(cmd == nullptr) exit(0);
        setpgrp();
        prepare();
        cmd->execute();
        cleanup();
        exit(0);
    }else waitpid(c_pid, nullptr, 0);
}
void RedirectionCommand::prepare() {
    int flags = append ? (O_CREAT | O_WRONLY | O_APPEND) :
                            (O_CREAT | O_WRONLY | O_TRUNC);
    close(1);
    fd = open(out_file_path, flags, 0666);
}
void RedirectionCommand::cleanup() {
    if(fd > 0) close(fd);
}
PipeCommand::PipeCommand(const char* cmd_line):Command(cmd_line){}
void PipeCommand::execute(){}

void GetCurrDirCommand::execute(){
// TODO: test
    cout << _getCurrDir() << endl;
}

void GetDirContentCommand::execute(){
// TODO: add your implementation
    struct dirent **namelist;
    int n = scandir(".", &namelist, 0, alphasort);
    if (n < 0) perror("smash error: scandir failed");
    else{
        for (int i = 0; i < n; i++){
            cout << namelist[i]->d_name << endl;
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

    cout << "smash pid is " << getpid() << endl;
}

ChangeDirCommand::ChangeDirCommand(const char *cmd_line, string plastPwd):BuiltInCommand(cmd_line),old(plastPwd){}
void ChangeDirCommand::execute(){
    string oldpath = get_current_dir_name();
    const vector<string> s=explode(cmd_line,' ');
    if(args[2]){                                                    // Too many args
        perror("smash error: cd: too many arguments");
    }else{                                                          //Less then 2 arguments
        if(args[1]){                                                //Exacly 2 arguments
            char p[s[1].length()+1];
            for (unsigned int i = 0; i < sizeof(p); i++) {
                p[i] = s[1][i];
            }
            p[s[1].length()]=0;
            if(s[1]=="-"){
                if(old==""){
                    cerr<<"smash error: cd: OLDPWD not set"<<endl;
                }else{
                    char p[old.length()+1];
                    for (unsigned int i = 0; i < old.length()+1; i++) {
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

void JobsCommand::execute(){
// TODO: add your implementation
    jobs->printJobsList();
}

void ForegroundCommand::execute(){
// TODO: test
    jobs->removeFinishedJobs();
    if(args[1]){
        if(_isNumber(string(args[1]))) job_id = stoi(args[1]);
        else{
            perror("smash error: fg: invalid arguments");
            return;
        }
    }
    JobsList::JobEntry* job = (job_id > 0) ? jobs->getJobById(job_id): jobs->getLastJob(&job_id);
    if(!job){
        string error_s;
        if(!job_id)
            error_s = string("smash error: fg: jobs list is empty");
        else
            error_s = string("smash error: fg: job-id ",job_id) + string( " does not exist");
        perror(error_s.c_str());
        return;
    }
    cout << cmd_line.c_str() << " : " << job->pid << endl;
    if(job->execution_state == Waiting){
        jobs->switchJobOn(job, true);
    }else{
        job->cmd->type = Foreground;
        jobs->setFg(job);
    }
    jobs->waitForJob(job);
}

void QuitCommand::execute(){
    if(args[1] && strcmp(args[1],"kill") == 0){
        cout << "sending SIGKILL signal to " << smash_p->jobs.free_job_ids.back() -1 << " jobs:" << endl;
        smash_p->jobs.killAllJobs();
    }
    //Kill smash
    exit(0);
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
//Parse arguments
    char* args[COMMAND_MAX_ARGS];
    if(!_parseCommandLine(cmd_line, args)) return nullptr;
    string cmd_s(args[0]);
    if(_isBackgroundCommand(args[0])) _removeBackgroundSign(args[0]);
    if(_isPipeCommand(cmd_line)){
        return new PipeCommand(cmd_line);
    }
    else if(_isRedirectonCommand(cmd_line)){
        return new RedirectionCommand(cmd_line, this);
    }
    else if ((string("pwd").find(cmd_s) && (args[0][3] == ' ')) || (cmd_s == "pwd")){
        return new GetCurrDirCommand(cmd_line); //Need to change the output
    }
    else if ((string("chprompt").find(cmd_s) && (args[0][8] == ' ')) || (cmd_s == "chprompt")){
        return new ChangePromptCommand(cmd_line, this); //Need to change the input
    }
       else if ((string("ls").find(cmd_s) && (args[0][2] == ' ')) || (cmd_s == "ls")){
        return new GetDirContentCommand(cmd_line); //Need to change the output
    }
    else if ((string("showpid").find(cmd_s) && (args[0][7] == ' ')) || (cmd_s == "showpid")) {
        return new ShowPidCommand(cmd_line); //Need to change the output
    }
    else if ((string("cd").find(cmd_s) && (args[0][2] == ' ')) || (cmd_s == "cd")) {
        return new ChangeDirCommand(cmd_line, oldp); //Need to change the input
    }
    else if ((string("jobs").find(cmd_s) && (args[0][4] == ' ')) || (cmd_s == "jobs")){
        return new JobsCommand(cmd_line, &jobs); //Need to change the input + output
    }
    else if ((string("fg").find(cmd_s) && (args[0][2] == ' ')) || (cmd_s == "fg")){
        return new ForegroundCommand(cmd_line, &jobs); //Need to change the input
    }
    else if ((string("quit").find(cmd_s) && (args[0][4] == ' ')) || (cmd_s == "quit")){
        return new QuitCommand(cmd_line, this); //Need to change the input
    }
    else {
        //cout << "INFO: Executing with bash." << endl;
    return new ExternalCommand(cmd_line, this); //Need to change the input + output
    }
}

void SmallShell::executeCommand(const char *cmd_line) {
    // TODO: Add your implementation here
    // for example:


    Command* cmd = CreateCommand(cmd_line);
    //jobs.removeFinishedJobs();
    if(cmd) cmd->execute();
//    delete cmd;

    // Please note that you must fork smash process for some commands (e.g., external commands....)
}

