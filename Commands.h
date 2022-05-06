#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <algorithm>
#include <sys/wait.h>
#include <unistd.h>
#include <cassert>
#include <cstring>
#include <iostream>
#include <sstream>

using namespace std;
#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define PRINT_SMASH_ERROR_AND_RETURN(message)  do { \
    cerr << "smash error: " << getName() << ": " << (message) << endl; \
    std::cerr.flush();\
    return;} while(0)

#define SYS_CALL_ERROR_MESSAGE(name) do{\
    string ret =  "smash error: " + string(name) + " failed" ; \
    perror(ret.c_str());     \
   return;} while(0)

#define FAILURE -1

int _parseCommandLine(const char *cmd_line, char **args);

class Command {
    string name;
    char **args;
    int argsCount;
    char *cmd_line;
    pid_t pid;
public:
    Command(const char *cmd_line) {
        args = new char *[COMMAND_MAX_ARGS];
        argsCount = _parseCommandLine(cmd_line, args);
        name = string(args[0]);
        cmd_line = cmd_line;
    }

    virtual ~Command() {
        for (int i = 0; i < argsCount; i++)
            free(args[i]);
        delete[] args;
        delete cmd_line;
    }

    void setPid(pid_t _pid = getpid()) {
        pid = _pid;
    }

    pid_t getPid() {
        return pid;
    }

    virtual void execute() = 0;

    //todo virtual void prepare();
    //todo virtual void cleanup();
    // TODO: Add your extra methods if needed
    char **getArgs() {
        return args;
    }

    string getCmdLineAsString() const {
        return string(cmd_line);
    }

    char* getCmdLine() const{
        return cmd_line;
    }

    int getArgsCount() const {
        return argsCount;
    }

    string getName() const {
        return name;
    }
};
class SmallShell;

class BuiltInCommand : public Command {
    pid_t smash_pid;
public:
    BuiltInCommand(const char *cmd_line): Command(cmd_line){}

    virtual ~BuiltInCommand() = default;
};

class ExternalCommand : public Command {

public:
    ExternalCommand(const char *cmd_line) : Command(cmd_line) {}

    virtual ~ExternalCommand() = default;

    void execute() override;

};

class PipeCommand : public Command {
public:
    PipeCommand(const char *cmd_line): Command(cmd_line){}

    virtual ~PipeCommand() {}

    void execute() override;
};

class RedirectionCommand : public Command {
    // TODO: Add your data members
public:
    explicit RedirectionCommand(const char *cmd_line) : Command(cmd_line) {}

    virtual ~RedirectionCommand() {}

    void execute() override;
    //void prepare() override;
    //void cleanup() override;
};

class ChangePromptCommand : public BuiltInCommand {
    string *prompt;
public:
    ChangePromptCommand(const char *cmd_line, string *prompt) : BuiltInCommand(cmd_line), prompt(prompt) {}

    void execute() override;

};

class ChangeDirCommand : public BuiltInCommand {
    string plastPwd;
public:
    ChangeDirCommand(const char *cmd_line, string plastPwd) : BuiltInCommand(cmd_line), plastPwd(plastPwd) {}

    virtual ~ChangeDirCommand() {
    }

    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

    virtual ~GetCurrDirCommand() {}

    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

    virtual ~ShowPidCommand() {}

    void execute() override;
};

class JobsList;

class QuitCommand : public BuiltInCommand {
    JobsList *jobs;
public:
    QuitCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs(jobs) {}

    virtual ~QuitCommand() {}

    void execute() override;
};

class JobsList {
public:
    class JobEntry;
private:
    vector<JobEntry *> list;
    int currJobId;
public:
    JobsList() : currJobId(1) {}

    ~JobsList() = default;

    void addJob(Command *cmd, int jobId, pid_t pid, bool isStopped = false) {
        JobEntry *job = new JobEntry(pid, jobId, cmd, isStopped, time(nullptr));
        list.push_back(job);
    }

    int getJobIdToSet() {
        if(list.empty())
            return 1;
        currJobId = (*list.begin())->getJobId();
        for (auto job: list) {
            if(currJobId<job->getJobId())
                currJobId=job->getJobId();
        }
        return currJobId++;
    }

    void printJobsList() {
        std::sort(list.begin(), list.end());
        for (auto job: list) {
            cout << "[" << job->getJobId() << "] " << job->getCmdLine() << " : " << job->getProcessId() << " "
                 << difftime(time(nullptr), job->getTime()) << " secs ";
            if (job->isStoppedJob())
                cout << "(stopped)";
            cout << endl;
        }
    }

    static bool sortJobEntryByTime(JobEntry *a, JobEntry *b) {
        return a->sortTime(b);
    }

    void killAllJobs() {
        removeFinishedJobs();
        std::sort(list.begin(), list.end());
        cout << "sending SIGKILL signal to " << list.size() << " jobs:" << endl;
        for (JobEntry *job: list) {
            pid_t pid = job->getProcessId();
            cout << pid << ": " << job->getCmdLine() << endl;
            if (kill(pid, SIGKILL) == -1)
                perror("smash error: kill failed");
        }
    }

    bool jobExist(int jobId) {
        for (JobEntry *job: list)
            if (job->getJobId() == jobId)
                return true;
        return false;
    }

    void removeFinishedJobs() {
        int pos = 0;
        for (JobEntry *job: list) {
            if (waitpid(job->getProcessId(), nullptr, WNOHANG) > 0) {
                removeJobByPos(pos);
                pos--;
            }
            pos++;
        }
    }

    JobEntry *getJobById(int jobId) {
        auto job = list.begin();
        while (job != list.end()) {
            if ((*job)->getJobId() == jobId)
                return *job;
            job++;
        }
        return nullptr;
    }

    void removeJobByPos(int pos) {
        delete list[pos];
        list.erase(list.begin() + pos);
    }

    void removeJobById(int jobId) {
        for (int pos=0; pos<list.size();pos++) {
            JobEntry* job =list[pos];
            if (jobId == job->getJobId()) {
                delete list[pos];
                list.erase(list.begin() + pos);
            }
            pos++;
        }
    }

    JobEntry *getLastJob(int *lastJobId);

    JobEntry *getLastStoppedJob(int *jobId) {

        sort(list.begin(), list.end(), sortJobEntryByTime);
        int i = list.size();
        while (i > 0) {
            if (list[i - 1]->isStoppedJob()) {
                *jobId = list[i - 1]->getJobId();
                return list[i - 1];
            }
            i--;
        }
        return nullptr;
    }

    JobEntry *getMaxJobById() {
        auto max = list.begin();
        auto job = list.begin();
        while (job != list.end()) {
            if (max < job)
                max = job;
            job++;
        }
        return *max;
    }

    class JobEntry {
        int jobId;
        Command *cmd;
        time_t timeInserted;
        bool isStopped;
    public:
        JobEntry(int pid, int jobId, Command *cmd, bool isStopped, time_t timeInserted = time(nullptr))
                : jobId(jobId), cmd(cmd),
                  isStopped(isStopped),
                  timeInserted(timeInserted) {
            cmd->setPid(pid);
        }

        int getProcessId() {
            return cmd->getPid();
        }

        ~JobEntry() = default;

        bool operator<(const JobEntry &other) {
            return jobId < other.jobId;
        }


        int getJobId() {
            return jobId;
        }

        char *getCommandName() {
            return cmd->getArgs()[0];
        }

        string getCmdLine() {
            return cmd->getCmdLineAsString();
        }

        Command *getCommand() {
            return cmd;
        }

        time_t getTime() const {
            return timeInserted;
        }

        bool isStoppedJob() {
            return isStopped;
        }

        void setNotStopped() {
            isStopped = false;
        }

        bool sortTime(const JobEntry *other) {
            return difftime(getTime(), other->getTime()) > 0;
        }
    };
};


class JobsCommand : public BuiltInCommand {
    JobsList *jobs;
public:
    JobsCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs(jobs) {}

    virtual ~JobsCommand() {}

    void execute() override;
};

class KillCommand : public BuiltInCommand {
    JobsList *jobs;
public:
    KillCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs(jobs) {}

    virtual ~KillCommand() {}

    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    JobsList *jobs;
public:
    ForegroundCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs(jobs) {}

    virtual ~ForegroundCommand() {}

    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
    JobsList *jobs;
public:
    BackgroundCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs(jobs) {}

    virtual ~BackgroundCommand() {}

    void execute() override;
};

class TailCommand : public BuiltInCommand {
public:
    TailCommand(const char *cmd_line);

    virtual ~TailCommand() {}

    void execute() override;
};

class TouchCommand : public BuiltInCommand {
public:
    TouchCommand(const char *cmd_line);

    virtual ~TouchCommand() {}

    void execute() override;
};


class SmallShell {
private:
    string prompt;
    string plastPwd;
    JobsList *jobs;
    JobEntry* currForegroundCommand;

    SmallShell();

public:
    //need to delete finished jobs before any job Insertion
    Command *CreateCommand(const char *cmd_line);

    SmallShell(SmallShell const &) = delete; // disable copy ctor
    void operator=(SmallShell const &) = delete; // disable = operator
    static SmallShell &getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }

    ~SmallShell();

//hagai: need to delete finished jobs before any execute
    void executeCommand(const char *cmd_line);

    void setJobToForeground(char* cmd_line){
       JobsList::JobEntry job(getpid(),getJobIdToSet(),cmd,time(nullptr),isStopped);
        *currForegroundCommand= job;

    }

    string getPrompt() {
        return prompt;
    }

    void setPlastPwd(char *lastPath) {
        plastPwd = string(lastPath);
    }

    string getPlastPwd() {
        return plastPwd;
    }

    void setForegroundPidFromFather(pid_t pid) {
        currForegroundCommand->setPid(pid);
    }

    pid_t getPid() {
        return pid;
    }

    JobsList *getJobList() {
        return jobs;
    }

    int getForegroundJobId() {
        return fgJobId;
    }

    Command *getForegroundCommand() {
        return currForegroundCommand;
    }

    void resetForegroundJob() {
        currForegroundCommand = nullptr;
    }
    // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_
