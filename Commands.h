#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <sys/wait.h>
#include <numeric>
#include <vector>
#include <string>
#include <map>
#include <list>
#include <iostream>
#include <assert.h>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define MAX_NUM_PROC (100)
#define COMMAND_MAX_ARGS (20)
#define HISTORY_MAX_RECORDS (50)

using std::string;
using std::vector;
using std::map;
using std::list;
using std::istream;
using std::ostream;
enum Type {Foreground, Background};
enum ExecState {/*Ready,*/ Running, Waiting};

class SmallShell;
class Command {
    // TODO: Add your data members
public:
    Type type;
    string  cmd_line;
    string  orig_cmd_line;
    char*   args[COMMAND_MAX_ARGS];
    Command(const char* cmd_line, string orig_cmd_line):
        cmd_line(string(cmd_line)){
            if(orig_cmd_line == "") this->orig_cmd_line  = string(cmd_line);
            else this->orig_cmd_line = orig_cmd_line;
        }
    virtual ~Command() {}
    virtual void execute() = 0;
    virtual void prepare() {}
    virtual void cleanup() {}
// TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char* cmd_line, string orig_cmd);
    virtual ~BuiltInCommand() {}
    void execute() override;
};

class ExternalCommand : public Command {
private:
     SmallShell* smash_p;
     string run_cmd;
public:
    ExternalCommand(const char* cmd_line, SmallShell* smash_p, string orig_cmd);
    virtual ~ExternalCommand() {}
    void execute() override;
};

class PipeCommand : public Command {
// TODO: Add your data members
    bool errPipe;
    SmallShell* smash_p;
    Command* cmd1;
    Command* cmd2;
public:
    PipeCommand(const char* cmd_line,SmallShell* smash_p, string orig_cmd);
    virtual ~PipeCommand() {}
    void execute() override;
};

class RedirectionCommand : public Command {
// TODO: Add your data members
    bool append;
    int fd;
    int old_fd;
    string out_file_path;
    Command* cmd;
    SmallShell* smash_p;
public:
    explicit RedirectionCommand(const char* cmd_line, SmallShell* smash_p, string orig_cmd);
    virtual ~RedirectionCommand() {}
    void execute() override;
    void prepare() override;
    void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand {
// TODO: Add your data members
public:
    std::string old;
    ChangeDirCommand(const char* cmd_line, std::string plastPwd, string orig_cmd);
    virtual ~ChangeDirCommand() {}
    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(const char* cmd_line, string orig_cmd):
    BuiltInCommand(cmd_line, orig_cmd) {}
    virtual ~GetCurrDirCommand() {}
    void execute() override;
};

class GetDirContentCommand : public BuiltInCommand {
public:
    GetDirContentCommand(const char* cmd_line, string orig_cmd):
    BuiltInCommand(cmd_line, orig_cmd) {}
    virtual ~GetDirContentCommand() {}
    void execute() override;
};

class ChangePromptCommand : public BuiltInCommand {
protected:
    SmallShell* smash_p;
public:
    ChangePromptCommand(const char* cmd_line, SmallShell* smash_p, string orig_cmd):
    BuiltInCommand(cmd_line, orig_cmd),
    smash_p(smash_p) {}
    virtual ~ChangePromptCommand() {}
    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    SmallShell* smash_p;
    ShowPidCommand(const char* cmd_line, string orig_cmd, SmallShell* smash_p):
    BuiltInCommand(cmd_line, orig_cmd), smash_p(smash_p){}
    virtual ~ShowPidCommand() {}
    void execute() override;
};

class JobsList;
class QuitCommand : public BuiltInCommand {
// TODO: Add your data members
private:
    SmallShell* smash_p;
public:
    QuitCommand(const char* cmd_line, SmallShell* smash_p, string orig_cmd):
    BuiltInCommand(cmd_line, orig_cmd),
    smash_p(smash_p) {}
    virtual ~QuitCommand() {}
    void execute() override;
};


class  JobsList{
public:
    class JobEntry { //Same concept as PCB
    // TODO: Add your data members
    public:
        int job_id;
        pid_t pid;
        ExecState execution_state;
        Command* cmd;
        time_t start_time;
        JobEntry(int job_id, pid_t pid, ExecState execution_state,Command* cmd):
            job_id(job_id), pid(pid), execution_state(execution_state),
            cmd(cmd), start_time(time(nullptr)) {}

    };
// TODO: Add your data members
    list<JobEntry*>     waiting_queue;
    list<JobEntry*>     running_queue;
    //vector<int>         free_job_ids;
    map<int,JobEntry*>  jobs_map;
    JobEntry*           fg_job;
    int                 jobs_n;
    int _getValidJobId(){
        //int ret = free_job_ids.back();
        //free_job_ids.pop_back();
        //return ret;
        jobs_n++;
        for(int i = MAX_NUM_PROC; i > 0; i--) {
            if(jobs_map[i] != nullptr){
                return ++i;
            }
        }
        assert(jobs_n == 1);
        return jobs_n;
    }
public:
    JobsList(): jobs_map(), jobs_n(0){ //free_job_ids(), jobs_map(){
        jobs_map[0] = nullptr;
        for(int i = MAX_NUM_PROC; i > 0; i--) {
            //free_job_ids.push_back(i);
            jobs_map[i] = nullptr;
        }
    }
    ~JobsList() {}
    JobEntry* addJob(Command* cmd,  pid_t pid, bool isStopped = false){
        int new_job_id = _getValidJobId();
        JobEntry* new_job;
        if(isStopped){
            new_job = new JobEntry(new_job_id, pid, Waiting, cmd);
            waiting_queue.push_back(new_job);
        }
        else {
            new_job = new JobEntry(new_job_id, pid, Running, cmd);
            running_queue.push_back(new_job);
        }

        jobs_map[new_job_id] = new_job;
        //jobs_n++;
        return new_job;
    }

    void printJobsList(){
        removeFinishedJobs();
        for(int i = 1; i <= MAX_NUM_PROC; i++){
            if(jobs_map[i]){
                JobEntry job = *jobs_map[i];
                const char * stopped = (job.execution_state ==  Waiting)  ? "(stopped)" : "";
                std::cout   << "[" << job.job_id << "] "        \
                            <<  job.cmd->orig_cmd_line << " : "      \
                            << job.pid << " "                   \
                            << time(nullptr) - job.start_time   \
                            << " secs " << stopped << std::endl;
            }
        }
    }
    void killAllJobs(){
        for(int i = 1; i <= MAX_NUM_PROC; i++){
            killJobById(i);
            removeJobById(i);
        }
    }
    void removeFinishedJobs(){
        int wstatus = -1;
        for(int i = 1; i <= MAX_NUM_PROC; i++){
            JobEntry* job = jobs_map[i];
            if((job != nullptr) && (waitpid(job->pid, &wstatus , WNOHANG) > 0)){
                removeJobById(job->job_id);
            }
        }
    }
    JobEntry * getJobById(int jobId){
        return jobs_map[jobId];
    }
    void removeJobById(int jobId){
        JobEntry* job;
        if(jobId > 0){
            job = jobs_map[jobId];
            if(job){
                if(job->execution_state == Waiting) waiting_queue.remove(job);
                else running_queue.remove(job);
                jobs_map.erase(jobId);
                //free_job_ids.push_back(jobId);
                if(job == fg_job) fg_job = nullptr;
                else jobs_n--;
                delete job;
            }
        }else{
            job = fg_job;
            if(job){
                if(job->execution_state == Waiting) waiting_queue.remove(job);
                else running_queue.remove(job);
                delete job;
                fg_job = nullptr;
            }
        }
    }
    void removeJob(JobEntry* job){
        if(!job) { return; }
        else removeJobById(job->job_id);
    }
    void killJobById(int job_id){
        JobEntry* job_to_kill = jobs_map[job_id];
        if(job_to_kill){
            std::cout << job_to_kill->pid << ": " << job_to_kill->cmd->cmd_line << std::endl;
            kill(job_to_kill->pid*(-1),SIGKILL);
        }
    }
    JobEntry * getLastJob(int* lastJobId = nullptr){
        for(int i = MAX_NUM_PROC; i > 0; i--)
            if(jobs_map[i] != nullptr){
                if(lastJobId) *lastJobId = i;
                return jobs_map[i];
            }
        return nullptr;
    }
    JobEntry *getLastStoppedJob(int *jobId = nullptr){
        if(waiting_queue.back() != nullptr && jobId) *jobId = waiting_queue.back()->job_id;
        return waiting_queue.back();
    }
    JobEntry * getFg(int* lastJobId = nullptr){
        return fg_job;
    }
    JobEntry * setFg(JobEntry* job = nullptr){
        assert(fg_job == nullptr);
        assert(job->cmd->type == Foreground);
        fg_job = job;
        return fg_job;
    }
    JobEntry * popFg(int* lastJobId = nullptr){
        auto fg = getFg();
        fg_job = nullptr;
        return fg;
    }
// TODO: Add extra methods or modify exisitng ones as needed
    void insertJob(JobEntry* job){
        assert(jobs_map[job->job_id] == nullptr || jobs_map[job->job_id] == job);
        if(job->job_id == 0){
            insertNewJob(job);
        }else{
            assert(jobs_map[job->job_id] == job);
            auto proc_list = job->execution_state == Waiting ? &waiting_queue : &running_queue;
            proc_list->push_back(job);
        }
    }
    void insertNewJob(JobEntry* job){
        assert(jobs_map[job->job_id] == nullptr);
        job->job_id = _getValidJobId();
        //jobs_n++;
        jobs_map[job->job_id] = job;
        auto proc_list = job->execution_state == Waiting ? &waiting_queue : &running_queue;
        proc_list->push_back(job);
    }
    void switchJobOff(JobEntry* job){
        if(!job) return;
        if(fg_job == job) {
            fg_job = nullptr;
            //jobs_n++;
            if(job->job_id == 0){
                job->job_id = _getValidJobId();
                jobs_map[job->job_id] = job;
            }
        }
        running_queue.remove(job);
        waiting_queue.push_back(job);
        job->execution_state = Waiting;
        kill(job->pid*(-1), SIGSTOP);
        std::cout << "smash: process " << job->pid << " was stopped" << std::endl;
    }
    void switchJobOn(JobEntry* job, bool move_to_fg = false){
        assert((move_to_fg && fg_job) == 0);
        if(move_to_fg){
            assert(fg_job == nullptr);
            job->cmd->type = Foreground;
            fg_job = job;
            //jobs_n--;
        }
        waiting_queue.remove(job);
        running_queue.push_back(job);
        job->execution_state = Running;
        kill(job->pid*(-1), SIGCONT);
    }
    JobsList::JobEntry* getJobByPid(int pid){
        for(auto job : jobs_map)
        {
            if(job.second && job.second->pid == pid) return job.second;
        }
        return nullptr;
    }
    void waitForJob(JobsList::JobEntry* job){
        assert(job->cmd->type == Foreground);
        assert(fg_job == job);
        //std::cerr << "Waiting for pid:" << job->pid << std::endl;
        int wstatus = -1;         
        int w = waitpid(job->pid, &wstatus,  WUNTRACED);
        if (w == -1) {
            std::cerr << job->cmd->cmd_line << " pid:" << job->pid << std::endl;
            perror("waitpid");
        }
        if (WIFEXITED(wstatus) || WIFSIGNALED(wstatus)) removeJob(job);
        else if (WIFSTOPPED(wstatus)){
            //std::cout << "here";
            job->execution_state = Waiting;
            job->cmd->type = Background;
            insertJob(job);
            //break;
        }
    }
};


class JobsCommand : public BuiltInCommand {
// TODO: Add your data members
private:
    JobsList* jobs;
public:
    JobsCommand(const char* cmd_line, JobsList* jobs, string orig_cmd):
    BuiltInCommand(cmd_line, orig_cmd), jobs(jobs) {}
    virtual ~JobsCommand() {}
    void execute() override;
};

class KillCommand : public BuiltInCommand {
// TODO: Add your data members
    JobsList *jobs;
public:
    KillCommand(const char* cmd_line, JobsList *jobs, string orig_cmd);
    virtual ~KillCommand() {}
    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
// TODO: Add your data members
private:
    JobsList* jobs;
    int job_id;
    SmallShell* smash_p;
public:
    ForegroundCommand(const char* cmd_line, SmallShell* smash_p, JobsList* jobs, string orig_cmd, int job_id = 0):
        BuiltInCommand(cmd_line, orig_cmd), jobs(jobs), job_id(job_id), smash_p(smash_p){}
    virtual ~ForegroundCommand() {}
    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
// TODO: Add your data members
    JobsList *jobs;
public:
    BackgroundCommand(const char* cmd_line, JobsList* jobs, string orig_cmd);
    virtual ~BackgroundCommand() {}
    void execute() override;
};

// TODO: add more classes if needed
// maybe timeout ?

class SmallShell {
private:
// TODO: Add your data members
    string name;
    SmallShell();
public:
    static std::string oldp;
    JobsList jobs;
    pid_t   pid;
    Command *CreateCommand(const char* cmd_line, string orig_cmd = "");
    SmallShell(SmallShell const&)      = delete; // disable copy ctor
    void operator=(SmallShell const&)  = delete; // disable = operator
    static SmallShell& getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }
    ~SmallShell();
    void executeCommand(const char* cmd_line);
// TODO: add extra methods as needed
    const string getName();
    const string setName(const string new_name = "smash");
    //void addJob(Command* cmd,  pid_t pid, bool isStopped = false);
};

#endif //SMASH_COMMAND_H_
