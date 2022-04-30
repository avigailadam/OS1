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

int _parseCommandLine(const char *cmd_line, char **args);

class Command {
    string name;
    char **args;
    int argsCount;
    char *cmd_line;
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

    virtual void execute() = 0;

    //todo virtual void prepare();
    //todo virtual void cleanup();
    // TODO: Add your extra methods if needed
    char **getArgs() {
        return args;
    }

    string getCmdLine() const {
        return string(cmd_line);
    }

    int getArgsCount() const {
        return argsCount;
    }

    string getName() const {
        return name;
    }
};


class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char *cmd_line) : Command(cmd_line) {}

    virtual ~BuiltInCommand() = default;
};

class ExternalCommand : public Command {
public:
    ExternalCommand(const char *cmd_line) : Command(cmd_line) {}

    virtual ~ExternalCommand() = default;

    void execute() override;
};

class PipeCommand : public Command {
    // TODO: Add your data members
public:
    PipeCommand(const char *cmd_line);

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
    vector<JobEntry> list;
    int currJobId;
public:
    JobsList() : currJobId(1) {}

    ~JobsList();

    void addJob(Command *cmd, bool isStopped = false) {
        JobEntry job(getpid(), getJobIdToSet(), cmd, time(nullptr), isStopped);
        list.push_back(job);
    }

    int getJobIdToSet() {
        currJobId++;
        return currJobId - 1;
    }

    void printJobsList() {
        std::sort(list.begin(), list.end());
        for (auto job: list) {
            cout << "[" << job.getJobId() << "] " << job.getCommandName() << " : " << job.getProcessId() << " "
                 << difftime(time(nullptr), job.getTime()) << " secs ";
            if (job.isStoppedJob())
                cout << "(stopped)";
            cout << endl;
        }
    }

    static bool sortJobEntryByTime(JobEntry a, JobEntry b) {
        return a.sortTime(b);
    }

    void killAllJobs() {
        removeFinishedJobs();
        std::sort(list.begin(), list.end());
        cout << "sending SIGKILL signal to " << list.size() << " jobs:" << endl;
        for (JobEntry job: list) {
            pid_t pid = job.getProcessId();
            cout << pid << ": " << job.getCmd() << endl;
            if (kill(pid, SIGKILL) == -1)
                perror("smash error: kill failed");
        }
    }

    bool jobExist(int jobId) {
        for (JobEntry job: list)
            if (job.getJobId() == jobId)
                return true;
        return false;
    }

    void removeFinishedJobs() {
        int pos = 0;
        for (JobEntry job: list) {
            if (waitpid(job.getProcessId(), nullptr, WNOHANG) > 0) {
                removeJobByPos(pos);
                pos--;
            }
            pos++;
        }
    }

    JobEntry *getJobById(int jobId) {
        auto job = list.begin();
        while (job != list.end()) {
            if (job->getJobId() == jobId)
                return &(*job);
            job++;
        }
        return nullptr;
    }

    void removeJobByPos(int pos) {
        list.erase(list.begin() + pos);
    }

    void removeJobById(int jobId) {
        int pos = 0;
        for (JobEntry job: list) {
            if (jobId == job.getJobId())
                list.erase(list.begin() + pos);
            pos++;
        }
    }

    JobEntry *getLastJob(int *lastJobId);

    JobEntry *getLastStoppedJob(int *jobId) {

        sort(list.begin(), list.end(), sortJobEntryByTime);
        int i = list.size();
        while (i > 0) {
            if (list[i - 1].isStoppedJob()) {
                *jobId = list[i - 1].getJobId();
                return &list[i - 1];
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
        return &(*max);
    }

    class JobEntry {
        int processId;
        int jobId;
        Command *cmd;
        time_t timeInserted;
        bool isStopped;
    public:
        JobEntry(int processId, int jobId, Command *cmd, bool isStopped, time_t timeInserted = time(nullptr))
                : processId(processId),
                  jobId(jobId), cmd(cmd),
                  isStopped(isStopped),
                  timeInserted(timeInserted) {}

        int getProcessId() {
            return processId;
        }

        bool operator<(const JobEntry &other) {
            return jobId < other.jobId;
        }


        int getJobId() {
            return jobId;
        }

        char *getCommandName() {
            return cmd->getArgs()[0];
        }

        string getCmd() {
            return cmd->getCmdLine();
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

        bool sortTime(const JobEntry other) {
            return difftime(getTime(), other.getTime()) > 0;
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

    SmallShell() ;

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
    void setPlastPwd(char* lastPath){
        plastPwd =string(lastPath);
    }
    string getPlastPwd(){
        return plastPwd;
    }
    void setPrompt(const string p) {
        prompt = p;
    }

    pid_t getPid() {
        return getpid();
    }

    JobsList *getJobList() {
        return jobs;
    }

    // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_
