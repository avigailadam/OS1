#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

int _parseCommandLine(const char *cmd_line, char **args);

class Command {
    string name;
    char **args;
    int argsCount;
public:
    Command(const char *cmd_line) {
        char *args[COMMAND_MAX_ARGS];
        int argsCount = _parseCommandLine(cmd_line, args);
        name = string(args[0]);
    }

    virtual ~Command() {
        delete[] args;
    }

    virtual void execute() = 0;

    //todo virtual void prepare();
    //todo virtual void cleanup();
    // TODO: Add your extra methods if needed
    char **getArgs() {
        return args;
    }

    int getArgsCount() {
        return argsCount;
    }

    string getName() {
        return name;
    }
};


class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char *cmd_line);

    virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
public:
    ExternalCommand(const char *cmd_line);

    virtual ~ExternalCommand() {}

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
    char **plastPwd;
public:
    ChangeDirCommand(const char *cmd_line, char **plastPwd) : BuiltInCommand(cmd_line), plastPwd(plastPwd) {}

    virtual ~ChangeDirCommand() {
        delete plastPwd;
    }

    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

    virtual ~GetCurrDirCommand() {}

    void execute() override {

    }
};

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

    virtual ~ShowPidCommand() {}

    void execute() override;
};

class JobsList;

class QuitCommand : public BuiltInCommand {
// TODO: Add your data members public:
    QuitCommand(const char *cmd_line, JobsList *jobs);

    virtual ~QuitCommand() {}

    void execute() override;
};


class JobsList {
public:
    class JobEntry {
        // TODO: Add your data members
    };
    // TODO: Add your data members
public:
    JobsList();

    ~JobsList();

    void addJob(Command *cmd, bool isStopped = false);

    void printJobsList();

    void killAllJobs();

    void removeFinishedJobs();

    JobEntry *getJobById(int jobId);

    void removeJobById(int jobId);

    JobEntry *getLastJob(int *lastJobId);

    JobEntry *getLastStoppedJob(int *jobId);
    // TODO: Add extra methods or modify exisitng ones as needed
};

class JobsCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    JobsCommand(const char *cmd_line, JobsList *jobs);

    virtual ~JobsCommand() {}

    void execute() override;
};

class KillCommand : public BuiltInCommand {
    JobsList *jobs;
public:
    KillCommand(const char *cmd_line, JobsList *jobs);

    virtual ~KillCommand() {}

    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    ForegroundCommand(const char *cmd_line, JobsList *jobs);

    virtual ~ForegroundCommand() {}

    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    BackgroundCommand(const char *cmd_line, JobsList *jobs);

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
    string path;
    char **plastPwd;
    JobsList *jobs;

    SmallShell();

public:
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

    string getPrompt() {
        return prompt;
    }


    void setPrompt(const string p) {
        prompt = p;
    }

    int getPid() {
        return getpid();
    }

    JobsList *getJobList() {
        return jobs;
    }

    // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_
