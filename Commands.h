#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <sys/wait.h>
#include <numeric>
#include <vector>
#include <string>
#include <map>
#include <list>
#include <iostream>


#define COMMAND_ARGS_MAX_LENGTH (200)
#define MAX_NUM_PROC (100)
#define COMMAND_MAX_ARGS (20)
#define HISTORY_MAX_RECORDS (50)

using std::string;
using std::vector;
using std::map;
using std::list;
enum Type {Foreground, Background};
enum ExecState {/*Ready,*/ Running, Waiting};

class SmallShell;
class Command {
    // TODO: Add your data members
public:
    Type type;
    string  cmd_line;
    char*   args[COMMAND_MAX_ARGS];
    Command(const char* cmd_line);
    virtual ~Command() {}
    virtual void execute() = 0;
//virtual void prepare();
//virtual void cleanup();
// TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
    //std::string[COMMAND_MAX_ARGS] args;
public:
    BuiltInCommand(const char* cmd_line):
    Command(cmd_line) {}
    virtual ~BuiltInCommand() {}
    void execute() override;
};

class ExternalCommand : public Command {
private:
     SmallShell* smash_p;
public:
    ExternalCommand(const char* cmd_line, SmallShell* smash_p):
            Command(cmd_line),
            smash_p(smash_p) {}
    virtual ~ExternalCommand() {}
    void execute() override;
};

class PipeCommand : public Command {
// TODO: Add your data members
public:
    PipeCommand(const char* cmd_line);
    virtual ~PipeCommand();
    void execute() override;
};

class RedirectionCommand : public Command {
// TODO: Add your data members
public:
    explicit RedirectionCommand(const char* cmd_line);
    virtual ~RedirectionCommand();
    void execute() override;
//void prepare() override;
//void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand {
// TODO: Add your data members
public:
    ChangeDirCommand(const char* cmd_line, char** plastPwd);
    virtual ~ChangeDirCommand() {}
    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(const char* cmd_line):
    BuiltInCommand(cmd_line) {}
    virtual ~GetCurrDirCommand() {}
    void execute() override;
};

class GetDirContentCommand : public BuiltInCommand {
public:
    GetDirContentCommand(const char* cmd_line):
    BuiltInCommand(cmd_line) {}
    virtual ~GetDirContentCommand() {}
    void execute() override;
};

class ChangePromptCommand : public BuiltInCommand {
protected:
    SmallShell* smash_p;
public:
    ChangePromptCommand(const char* cmd_line, SmallShell* smash_p):
    BuiltInCommand(cmd_line),
    smash_p(smash_p) {}
    virtual ~ChangePromptCommand() {}
    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const char* cmd_line);
    virtual ~ShowPidCommand() {}
    void execute() override;
};

class JobsList;
class QuitCommand : public BuiltInCommand {
// TODO: Add your data members
public:
    QuitCommand(const char* cmd_line, JobsList* jobs);
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
private:
// TODO: Add your data members
    list<JobEntry*>     waiting_queue;
    vector<int>         free_job_ids;
    map<int,JobEntry*>  jobs_map;
    int _getValidJobId()
    {
        int ret = free_job_ids.back();
        free_job_ids.pop_back();
        return ret;
    }
public:
    JobsList(): free_job_ids(MAX_NUM_PROC), jobs_map()
    {
        std::iota(free_job_ids.begin(),free_job_ids.end(),1);
    }
    ~JobsList() {}
    void addJob(Command* cmd,  pid_t pid, bool isStopped = false)
    {
        int new_job_id = MAX_NUM_PROC - _getValidJobId();
        ExecState state = isStopped ? Waiting : Running;
        JobEntry* new_job = new JobEntry(new_job_id, pid, state, cmd);
        waiting_queue.push_back(new_job);
        jobs_map[new_job_id] = new_job;

    }

    void printJobsList()
    {
        removeFinishedJobs();
        for(int i = 1; i <= MAX_NUM_PROC; i++)
        {
            if(jobs_map[i])
            {
                JobEntry job = *jobs_map[i];
                const char * stopped = (job.execution_state ==  Waiting)  ? "(stopped)" : "";
                std::cout   << "[" << job.job_id << "] "        \
                            <<  job.cmd->cmd_line << " : "      \
                            << job.pid << " "                   \
                            << time(nullptr) - job.start_time   \
                            << " secs " << stopped << "\n";
            }
        }
    }
    void killAllJobs()
    {
        for(int i = 1; i <= MAX_NUM_PROC; i++) removeJobById(i);
    }
    void removeFinishedJobs()
    {
        int *return_status = NULL;
        for(const JobEntry* job : waiting_queue)
        {
            if(waitpid(job->pid, return_status, WNOHANG) > 0)
            {
                removeJobById(job->job_id);
            }
        }
    }
    JobEntry * getJobById(int jobId)
    {
        return jobs_map[jobId];
    }
    void removeJobById(int jobId)
    {
        JobEntry* job = jobs_map[jobId];
        if(job)
        {
            waiting_queue.remove(job);
            jobs_map.erase(jobId);
            free_job_ids.push_back(jobId);
            delete job;
        }
    }
    JobEntry * getLastJob(int* lastJobId)
    {
        if(waiting_queue.back() != nullptr && lastJobId) *lastJobId = waiting_queue.back()->job_id;
        return waiting_queue.back();
    }
    JobEntry *getLastStoppedJob(int *jobId)
    {
        return getLastJob(jobId);
    }
// TODO: Add extra methods or modify exisitng ones as needed
};


class JobsCommand : public BuiltInCommand {
// TODO: Add your data members
private:
    JobsList* jobs;
public:
    JobsCommand(const char* cmd_line, JobsList* jobs):
    BuiltInCommand(cmd_line), jobs(jobs) {}
    virtual ~JobsCommand() {}
    void execute() override;
};

class KillCommand : public BuiltInCommand {
// TODO: Add your data members
public:
    KillCommand(const char* cmd_line, JobsList* jobs);
    virtual ~KillCommand() {}
    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
// TODO: Add your data members
private:
    JobsList* jobs;
    int job_id;

public:
    ForegroundCommand(const char* cmd_line, JobsList* jobs, int job_id = 0):
        BuiltInCommand(cmd_line), jobs(jobs), job_id(job_id) {}
    virtual ~ForegroundCommand() {}
    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
// TODO: Add your data members
public:
    BackgroundCommand(const char* cmd_line, JobsList* jobs);
    virtual ~BackgroundCommand() {}
    void execute() override;
};

// TODO: add more classes if needed
// maybe ls, timeout ?

class SmallShell {
private:
// TODO: Add your data members
    string name;
    SmallShell();
public:
    JobsList jobs;
    Command *CreateCommand(const char* cmd_line);
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
