#include <dirent.h>
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

bool _isPipeErrCommand(const char* cmd_line) {
    const string str(cmd_line);
    return (str[str.find_first_of("|")+1] == '&');
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

void _execCmd(string run_cmd){
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

string _makeCmdTimout(vector<string> cmd_line, time_t * time){
    if(cmd_line.size() < 3){
        return "";
    }
    try{
            time_t cmd_time = (time_t) stol(cmd_line[1]);
            if(cmd_time < 0){
                return "";
            }else{
                *time = cmd_time;
            }
    }
    catch(std::invalid_argument& e){
            return "";
    }
    assert((cmd_line[0] == string("timeout")));
    cmd_line[0] = "";
    cmd_line[1] = "";
    std::string s;
    for (const auto &piece : cmd_line){
        s += piece;
        s += " ";
    }
    return s;
}

SmallShell::SmallShell():
    name("smash"), jobs(), pid(getpid()),
    oldout_fd(-1){}

SmallShell::~SmallShell() {
// TODO: add your implementation
    jobs.killAllJobs();
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
BuiltInCommand::BuiltInCommand(const char* cmd_line, string orig_cmd):
    Command(cmd_line, orig_cmd) {
    //assert(_isBackgroundCommand(cmd_line) == false);//_removeBackgroundSign(new_cmd_line);
    type = Foreground;
    _parseCommandLine(cmd_line,args);
    this->cmd_line = string(cmd_line);
}
void BuiltInCommand::execute(){
// TODO: add your implementation
}
ExternalCommand::ExternalCommand(const char* cmd_line, SmallShell* smash_p, string orig_cmd):
        Command(cmd_line, orig_cmd),
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
    if(smash_p->pid == getpid()){
        pid_t c_pid = fork();
        if(c_pid > 0){
            if(smash_p->pid){
                JobsList::JobEntry* new_job;
                if(type == Foreground){
                    new_job = new JobsList::JobEntry(0, c_pid, Running, this);
                    smash_p->jobs.fg_job = new_job;
                    smash_p->jobs.waitForJob(new_job);
                }else{
                    smash_p->jobs.addJob(this, c_pid);
                }
            }
        }
        else{
            setpgrp();
            _execCmd(run_cmd);
        }
    }
    else{
        _execCmd(run_cmd);
    }
}
RedirectionCommand::RedirectionCommand(const char* cmd_line, SmallShell* smash_p, string orig_cmd):
    Command(cmd_line, orig_cmd), append(false), fd(-1), old_fd(-1), smash_p(smash_p){
        char cmd_line_cpy[strlen(cmd_line)];
        type = Foreground;
        strcpy(cmd_line_cpy, cmd_line);

        _removeFirstOfSign(cmd_line_cpy, ">");
        if(_isRedirectonCommand(cmd_line_cpy)){
            append = true;
            _removeFirstOfSign(cmd_line_cpy, ">");
        }

        if(_isBackgroundCommand(cmd_line_cpy)){
            type = Background;
            _removeFirstOfSign(cmd_line_cpy, "&");
        }

        int argn = _parseCommandLine(cmd_line_cpy, args);
        out_file_path = string(args[argn-1]);

        string  new_cmd(cmd_line_cpy);
        std::size_t pos = new_cmd.rfind(out_file_path);
        assert(pos != string::npos);
        new_cmd = new_cmd.substr(0, pos);
        if(type == Background) new_cmd += "&";
        cmd = smash_p->CreateCommand(new_cmd.c_str(), this->orig_cmd_line);
    }
void RedirectionCommand::execute(){
    prepare();
    if(fd > 0) cmd->execute();
    cleanup();
}
void RedirectionCommand::prepare() {
    int flags = append ? (O_CREAT | O_WRONLY | O_APPEND) :
                            (O_CREAT | O_WRONLY | O_TRUNC);
    fd = open(out_file_path.c_str(), flags, 0666);
    if(fd > 0){ //if open succeded
        smash_p->oldout_fd = dup(1); //save current stdout_fd in old_fd
        int temp = dup2(fd, 1); //close 1 and put there new file
        assert(temp == 1);
        close(fd);
    }else{
        perror("smash error: open failed");
        dup2(smash_p->oldout_fd,1); //copy stdout back to 1
        close(smash_p->oldout_fd);
        smash_p->oldout_fd = -1;
    }
}
void RedirectionCommand::cleanup() {
    if(fd > 0) {
        //close(1);
        dup2(smash_p->oldout_fd,1); //copy stdout back to 1
        close(smash_p->oldout_fd);
        smash_p->oldout_fd = -1;
    }

}
PipeCommand::PipeCommand(const char* cmd_line, SmallShell* smash_p, string orig_cmd):
    Command(cmd_line, orig_cmd), errPipe(false), smash_p(smash_p){
        char cmd_line_cpy[strlen(cmd_line)];
        type = Foreground;
        strcpy(cmd_line_cpy, cmd_line);

        if(_isPipeErrCommand(cmd_line_cpy)){
            _removeFirstOfSign(cmd_line_cpy, "&");
            errPipe = true;
        }

        if(_isBackgroundCommand(cmd_line_cpy)){
            _removeFirstOfSign(cmd_line_cpy, "&");
            type = Background;
        }

        string str(cmd_line_cpy);
        size_t idx = str.find("|");
        string cmd_line1(str.substr(0,idx));
        cmd1 = smash_p->CreateCommand(cmd_line1.c_str(), this->orig_cmd_line);
        string cmd_line2(str.substr(idx+1));
        cmd2 = smash_p->CreateCommand(cmd_line2.c_str(), this->orig_cmd_line);
    }
void PipeCommand::execute(){
    if(!(cmd1 && cmd2)) return;

    //DEBUG_PRINT << "Before" << endl;
    pid_t c_pid = fork();
    
    if(c_pid > 0){ //father = smash
        if(type == Foreground){
            JobsList::JobEntry * new_job = new JobsList::JobEntry(0, c_pid, Running, this);
            smash_p->jobs.fg_job = new_job;
            smash_p->jobs.waitForJob(new_job);
        }else{
            smash_p->jobs.addJob(this, c_pid);
        }
    }
    else{ //child = pipe process
        setpgrp();
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        int fd[2];
        pipe(fd); // first child out --> second child in
        pid_t c_pid1 = fork();
        if(c_pid1 < 0){
            perror("smash error: fork failed");
            exit(0);
        }
        if (c_pid1 == 0) {// first child
            close(fd[0]);
            if(errPipe) dup2(fd[1],2);
            else dup2(fd[1],1);
            close(fd[1]);
             //cout << "Here" << endl;
            if(cmd1) cmd1->execute();
            exit(0);
        }else{ //pipe process
            pid_t c_pid2 = fork();
            if(c_pid2 < 0){
                perror("smash error: fork failed");
                exit(0);
            }
            if (c_pid2 == 0) {// second child
                close(fd[1]);
                dup2(fd[0],0);
                close(fd[0]);
                if(cmd2) cmd2->execute();
                exit(0);
            }
            //pipe process
            else{
                close(fd[0]);
                close(fd[1]);
                waitpid(c_pid2, nullptr, WUNTRACED);
            }
            waitpid(c_pid1, nullptr, WUNTRACED);
        }
        //wait(NULL);
        exit(0);
    }
}

void GetCurrDirCommand::execute(){
// TODO: test
    cout << _getCurrDir() << endl;
}

void GetDirContentCommand::execute(){
// TODO: add your implementation
    struct dirent **namelist;
    int n = scandir(".", &namelist, 0, alphasort);
    if (n < 0) {
        perror("smash error: scandir failed");
        return;
    }
    vector<string> dir;
    while(n--){
        //DEBUG_PRINT << namelist[n]->d_name << endl;
        string d_name(namelist[n]->d_name);
        if((d_name != string(".")) && (d_name != string(".."))){
            dir.push_back(d_name);
            free(namelist[n]);
        }
    }
    free(namelist);
    std::sort(dir.begin(),dir.end());
    for(auto file : dir){
        cout << file << endl;
    }

}


void ChangePromptCommand::execute(){
// TODO: test
    if(!args[1]) smash_p->setName(); // no args -> set name to be "smash"
    else smash_p->setName(args[1]); // else -> set shell name to be the 1st argument
}

void ShowPidCommand::execute(){

    cout << "smash pid is " << smash_p->pid << endl;
}

ChangeDirCommand::ChangeDirCommand(const char *cmd_line, string plastPwd, string orig_cmd):
BuiltInCommand(cmd_line, orig_cmd),old(plastPwd){}
void ChangeDirCommand::execute(){
    std::string oldpath = get_current_dir_name();
    const vector<string> s=explode(cmd_line,' ');
    if(s.size() > 2){                                                    // Too many args
        cout << ("smash error: cd: too many arguments") <<endl;
    }else{                                                          //Less then 2 arguments
        if(args[1]){                                                //Exacly 2 arguments
            char p[s[1].length()+1];
            for (unsigned int i = 0; i < sizeof(p); i++) {
                p[i] = s[1][i];
            }
            p[s[1].length()]=0;
            if(s[1]=="-"){
                if(old==""){
                    cout << "smash error: cd: OLDPWD not set" << endl;
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


TimeoutCommand::TimeoutCommand(const char* cmd_line,SmallShell* shell,string orig_cmd)
    :Command(cmd_line,orig_cmd), shell(shell), stopped_time(0){
    const vector<string> s = explode(cmd_line,' ');
    char cmd_line_cpy[strlen(cmd_line)];
    type = Foreground;
    strcpy(cmd_line_cpy, cmd_line);

    if(_isBackgroundCommand(cmd_line_cpy)){
        type = Background;
        _removeFirstOfSign(cmd_line_cpy, "&");
    }
    string  new_cmd = _makeCmdTimout(s,&duration);
    if(new_cmd != ""){
        cmd = shell->CreateCommand(new_cmd.c_str(), orig_cmd);
        if(type == Background) cmd->type = Background;
    }else cmd = nullptr;
}
void TimeoutCommand::execute(){
    if(!cmd) {
        cout << "smash error: timeout: invalid arguments" << endl;
        return;
    }
    const vector<string> s = explode(cmd_line,' ');
    start_time = time(nullptr);
    pid = fork();
    if(pid > 0){ //Father will do it : wait the time needed and stop the child
        time_t last_alarm = (time_t) alarm((unsigned int)duration);
        TimedCommandsPair new_timed_cmd(start_time + duration, this);
        shell->timed_cmds.insert(new_timed_cmd);
        if(last_alarm > 0 && new_timed_cmd.first > (last_alarm + start_time)){
            //There are earlier alarms
            alarm((unsigned int)(last_alarm));
        }
        if(type == Foreground){
            JobsList::JobEntry* new_job = new JobsList::JobEntry(0, pid, Running, this);
            shell->jobs.fg_job = new_job;
            int wstatus = shell->jobs.waitForJob(new_job);
            if(WIFEXITED(wstatus) || WIFSIGNALED(wstatus)){
                return;
            }
            assert(0);
        }else{
            shell->jobs.addJob(this, pid);
        }
    }
    else{  //Child will do it untill the father stop him
        setpgrp();
        cmd->execute();
        exit(0);
    }
}

KillCommand::KillCommand(const char* cmd_line,JobsList *jobs, string orig_cmd):
    BuiltInCommand(cmd_line, orig_cmd),jobs(jobs){}
void KillCommand::execute(){
    const vector<string> s = explode(cmd_line,' ');
    jobs->removeFinishedJobs();
    if(s.size() != 3){
        cout << ("smash error: kill: invalid arguments") << endl;
        }
    else{
        int sig = -1;
        int id = -1;
        try{
            id = stoi(s[2]);
            sig = stoi(s[1]);
            if(!(sig < 0)){
                cout << ("smash error: kill: invalid arguments") << endl;
                return;
            }
            sig = sig * (-1);
            pid_t p;
            JobsList::JobEntry * job = jobs->getJobById(id);
            if(job!=nullptr){
                p = job->pid;
                if(kill(p*(-1),sig) == -1){
                    perror("smash error: kill failed");
                }else{
                    cout << "signal number " << sig << " was sent to pid " << p << endl;
                }
            }else{
                cout << "smash error: kill: job-id " << id << " does not exist" << endl;
            }
            return;
        }catch(std::invalid_argument& e){
            cout << ("smash error: kill: invalid arguments") << endl;
            return;
        }
    }
}

BackgroundCommand::BackgroundCommand(const char* cmd_line, JobsList* jobs, string orig_cmd):
    BuiltInCommand(cmd_line, orig_cmd),jobs(jobs){}
void BackgroundCommand::execute(){
    jobs->removeFinishedJobs();
    const vector<string> s = explode(cmd_line,' ');
    if(s.size()>2){ //Too much parameters
        cout << ("smash error: bg: invalid arguments") << endl;
        return;
    }else{ //Less then two parameters
        if(s.size()==2){ //exacly two parameters
            int id;
            //if(stringstream(s[1])<<id){
            try{
                id = stoi(s[1]);
                JobsList::JobEntry *job = jobs->getJobById(id);
                if(job){
                    if(job->execution_state==Running){
                        cout << "smash error: bg: job-id "  << id << " is already running in the background"<< endl;
                        return;
                    }
                    std::cout << job->cmd->orig_cmd_line << " : " << job->pid << std::endl;
                    job->execution_state = Running;
                    if(kill(job->pid*(-1),SIGCONT) < 0){
                        perror("smash error: kill failed");
                        return;
                    }

                }else{
                   cout << "smash error: bg: job-id " << id << " does not exist" << endl;
                   return;
                }
            }catch(std::invalid_argument& e){
                cout << ("smash error: bg: invalid arguments") << endl;
                return;
            }
        }else{
            if(s.size()==1){ // No Parameter
                int id;
                JobsList::JobEntry *job = jobs->getLastStoppedJob(&id);
                if(job){
                    cout << job->cmd->orig_cmd_line << " : " << job->pid << endl;
                    job->execution_state = Running;
                    job->start_time = time(nullptr);
                    if(kill(job->pid*(-1),SIGCONT) < 0){
                        perror("smash error: kill failed");
                        return;
                    }
                }else{
                    cout << ("smash error: bg: there is no stopped jobs to resume") << endl;
                    return;
                }
            }else{
                cout << ("smash error: bg: invalid arguments") << endl;
                return;
            }
        }
    }
    jobs->removeFinishedJobs();

}

void JobsCommand::execute(){
// TODO: add your implementation
    jobs->printJobsList();
}

void ForegroundCommand::execute(){
// TODO: test
    jobs->removeFinishedJobs();
    if(args[1] && !args[2]){
        try{
            job_id = stoi(args[1]);
            if(job_id <= 0){
                cout << string("smash error: fg: job-id ") + to_string(job_id) + string( " does not exist") << endl;
                return;
            }
        }catch(std::invalid_argument& e){
            cout << ("smash error: fg: invalid arguments") << endl;
            return;
        }
    }else if(args[1] && args[2]){
        cout << ("smash error: fg: invalid arguments") << endl;
        return;
    }

    JobsList::JobEntry* job = (job_id > 0) ? jobs->getJobById(job_id): jobs->getLastJob(&job_id);
    if(!job){
        string error_s = (job_id == 0)  ? string("smash error: fg: jobs list is empty") :
                                    string("smash error: fg: job-id ") + to_string(job_id) + string( " does not exist");
        cout << (error_s.c_str()) << endl;
        return;
    }
    cout << job->cmd->orig_cmd_line << " : " << job->pid << endl;
    if(job->execution_state == Waiting){
        jobs->switchJobOn(job, true);
    }else{
        job->cmd->type = Foreground;
        jobs->setFg(job);
    }
    if(getpid() == smash_p->pid) jobs->waitForJob(job);
}

void QuitCommand::execute(){
    smash_p->jobs.removeFinishedJobs();
    if(args[1] && strcmp(args[1],"kill") == 0){
        cout << "smash: sending SIGKILL signal to " << smash_p->jobs.jobs_n << " jobs:" << endl;
        smash_p->jobs.killAllJobs();
    }
    //Kill smash
    exit(0);
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line, string orig_cmd) {
//Parse arguments
    if(orig_cmd == "") orig_cmd = string(cmd_line);
    char* args[COMMAND_MAX_ARGS];
    if(!_parseCommandLine(cmd_line, args)) return nullptr;
    string cmd_s(args[0]);
    if(_isBackgroundCommand(args[0])) _removeBackgroundSign(args[0]);
    if(_isRedirectonCommand(cmd_line)){
        return new RedirectionCommand(cmd_line, this, orig_cmd);
    }
    else if((string("timeout").find(cmd_s) && (args[0][2] == ' ')) || (cmd_s == "timeout"))  {
        return new TimeoutCommand(cmd_line,this,orig_cmd); //Need to change the output
    }
    else if(_isPipeCommand(cmd_line)){
        return new PipeCommand(cmd_line, this, orig_cmd);
    }
    else if ((string("pwd").find(cmd_s) && (args[0][3] == ' ')) || (cmd_s == "pwd")){
        return new GetCurrDirCommand(cmd_line, orig_cmd); //Need to change the output
    }
    else if ((string("chprompt").find(cmd_s) && (args[0][8] == ' ')) || (cmd_s == "chprompt")){
        return new ChangePromptCommand(cmd_line, this, orig_cmd); //Need to change the input
    }
       else if ((string("ls").find(cmd_s) && (args[0][2] == ' ')) || (cmd_s == "ls")){
        return new GetDirContentCommand(cmd_line, orig_cmd); //Need to change the output
    }
    else if ((string("showpid").find(cmd_s) && (args[0][7] == ' ')) || (cmd_s == "showpid")) {
        return new ShowPidCommand(cmd_line, orig_cmd, this); //Need to change the output
    }
    else if ((string("cd").find(cmd_s) && (args[0][2] == ' ')) || (cmd_s == "cd")) {
        return new ChangeDirCommand(cmd_line, oldp, orig_cmd); //Need to change the input
    }
    else if ((string("jobs").find(cmd_s) && (args[0][4] == ' ')) || (cmd_s == "jobs")){
        return new JobsCommand(cmd_line, &jobs, orig_cmd); //Need to change the input + output
    }
    else if ((string("fg").find(cmd_s) && (args[0][2] == ' ')) || (cmd_s == "fg")){
        return new ForegroundCommand(cmd_line, this, &jobs, orig_cmd); //Need to change the input
    }
    else if ((string("quit").find(cmd_s) && (args[0][4] == ' ')) || (cmd_s == "quit")){
        return new QuitCommand(cmd_line, this, orig_cmd); //Need to change the input
    }
    else if ((string("kill").find(cmd_s) && (args[0][4] == ' ')) || (cmd_s == "kill")){
        return new KillCommand(cmd_line, &jobs, orig_cmd); //Need to change the input
    }
    else if ((string("bg").find(cmd_s) && (args[0][4] == ' ')) || (cmd_s == "bg")){
        return new BackgroundCommand(cmd_line, &jobs, orig_cmd); //Need to change the input
    }
    else {
        //DEBUG_PRINT << "Runing with bash" << endl;
    return new ExternalCommand(cmd_line, this, orig_cmd); //Need to change the input + output
    }
}

void SmallShell::executeCommand(const char *cmd_line) {
    // TODO: Add your implementation here
    Command* cmd = CreateCommand(cmd_line, "");
    //jobs.removeFinishedJobs();
    if(cmd) cmd->execute();
    //delete cmd;
}
