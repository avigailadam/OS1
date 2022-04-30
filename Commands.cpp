#include "Commands.h"

using namespace std;

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

#define PRINT_SMASH_ERROR_AND_RETURN(message)  do { \
    cerr << "smash error: " << getName() << ": " << (message) << endl; \
    return;} while(0)

#define SYS_CALL_ERROR_MESSAGE(name) do{\
    string ret =  "smash error: " + string(name) + " failed" ; \
    perror(ret.c_str());     \
   return;} while(0)

#define FAILURE -1


const std::string WHITESPACE = " \n\r\t\f\v";

string _ltrim(const std::string &s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string &s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s) {
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char *cmd_line, char **args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)));
    for (std::string s; iss >> s;) {
        args[i] = (char *) malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
        strcpy(args[i], s.c_str());
        args[++i] = nullptr;
    }
    return i;

    FUNC_EXIT()
}


bool _isBackgroundCommand(const char *cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char *cmd_line) {
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

// TODO: Add your implementation for classes in Commands.h

SmallShell::SmallShell() :plastPwd(""){
    prompt = "smash";
    jobs = new JobsList();
}

SmallShell::~SmallShell() {
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command *SmallShell::CreateCommand(const char *cmd_line) {

    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
    size_t size = string(cmd_line).find_last_not_of(WHITESPACE) + 1;
    if (_isBackgroundCommand(cmd_line))
        size--;
    char *built_in_cmd_line = new char[size];
    for (int i = 0; i < size; ++i)
        built_in_cmd_line[i] = cmd_line[i];
    built_in_cmd_line[size] = 0;
    if (firstWord.compare("pwd") == 0) {
        return new GetCurrDirCommand(built_in_cmd_line);
    } else if (firstWord.compare("showpid") == 0) {
        return new ShowPidCommand(built_in_cmd_line);
    } else if (firstWord.compare("chprompt") == 0) {
        return new ChangePromptCommand(built_in_cmd_line, &prompt);
    } else if (firstWord.compare("cd") == 0) {
        return new ChangeDirCommand(built_in_cmd_line, plastPwd);
    } else if (firstWord.compare("kill") == 0) {
        return new KillCommand(built_in_cmd_line, jobs);
    } else if (firstWord.compare("jobs") == 0) {
        return new JobsCommand(built_in_cmd_line, jobs);
    } else if (firstWord.compare("fg") == 0) {
        return new ForegroundCommand(built_in_cmd_line, jobs);
    } else if (firstWord.compare("bg") == 0) {
        return new BackgroundCommand(built_in_cmd_line, jobs);
    } else if (firstWord.compare("quit") == 0) {
        return new QuitCommand(built_in_cmd_line, jobs);
    } else {
        return new ExternalCommand(cmd_line);
    }

    return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
    Command *cmd = CreateCommand(cmd_line);
    cmd->execute();

}

void ChangePromptCommand::execute() {
    *prompt = getArgsCount() == 1 ? "smash" : string(getArgs()[1]);
}

void GetCurrDirCommand::execute() {
    char *pwd = get_current_dir_name();
    cout << pwd << endl;
}

void ShowPidCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    cout << "smash pid is " << smash.getPid() << endl;
}

void ChangeDirCommand::execute() {
    if (getArgsCount() > 2)
        PRINT_SMASH_ERROR_AND_RETURN("too many arguments");
    SmallShell &smash = SmallShell::getInstance();
    string path = string(getArgs()[1]);
    if (path.compare("-") == 0) {
        if (plastPwd == "")
            PRINT_SMASH_ERROR_AND_RETURN("OLDPWD not set");
        char *tmp = get_current_dir_name();
        if (chdir(smash.getPlastPwd().c_str()) == FAILURE) {
            SYS_CALL_ERROR_MESSAGE("chdir");
        }
        smash.setPlastPwd(tmp);
        return;
    }
    if (path.compare("\0") == 0)
        path = get_current_dir_name();
    char *lastPath = get_current_dir_name();
    if (chdir(path.c_str()) == FAILURE) {
        SYS_CALL_ERROR_MESSAGE("chdir");
    } else {
        smash.setPlastPwd(lastPath);
    }
}

void KillCommand::execute() {
    if (getArgsCount() != 3 || stoi(string(getArgs()[1])) >= 0)
        PRINT_SMASH_ERROR_AND_RETURN("invalid arguments");
    int sigNum = stoi(string(getArgs()[1]));
    sigNum *= (FAILURE);
    int jobId = stoi(string(getArgs()[2]));
    if (!jobs->jobExist(jobId))
        PRINT_SMASH_ERROR_AND_RETURN("job-id" + to_string(jobId) + "does not exist");
    int pid = jobs->getJobById(jobId)->getProcessId();
    if (kill(pid, sigNum) == FAILURE)
        SYS_CALL_ERROR_MESSAGE("kill");
    cout << "signal number " << sigNum << " was sent to pid " << pid << endl;
}

void JobsCommand::execute() {
    if (jobs->empty())
        return;
    jobs->removeFinishedJobs();
    jobs->printJobsList();
}

void ForegroundCommand::execute() {
    JobsList::JobEntry *currJob;
    if (getArgsCount() > 2 || getArgsCount() == 0)
        PRINT_SMASH_ERROR_AND_RETURN("invalid arguments");
    if (getArgsCount() == 1) {
        if (jobs->empty())
            PRINT_SMASH_ERROR_AND_RETURN("jobs list is empty");
        currJob = jobs->getMaxJobById();
    } else {
        int jobId = stoi(string(getArgs()[1]));
        currJob = jobs->getJobById(jobId);
        if (currJob == nullptr) {
            cerr << "smash error: fg: job-id" << jobId << "does not exists" << endl;
            return;
        }
    }
    pid_t pid = currJob->getProcessId();
    cout << currJob->getCmd() << " : " << pid << endl;
    if (kill(pid, SIGCONT) == FAILURE)
        SYS_CALL_ERROR_MESSAGE("kill");
    if (waitpid(pid, nullptr, 0) == FAILURE)
        SYS_CALL_ERROR_MESSAGE("waitpid");
    jobs->removeJobById(currJob->getJobId());
}

void BackgroundCommand::execute() {
    if (getArgsCount() > 2 || getArgsCount() == 0)
        PRINT_SMASH_ERROR_AND_RETURN("invalid arguments");
    JobsList::JobEntry *job;
    if (getArgsCount() == 1) {
        int *jobId;
        job = jobs->getLastStoppedJob(jobId);
        if (job == nullptr)
            PRINT_SMASH_ERROR_AND_RETURN("there is no stopped jobs to resume");
    } else {
        job = jobs->getJobById(stoi(string((getArgs()[1]))));
        if (job == nullptr) {
            cerr << "smash error: bg: job-id " << getArgs()[1] << " does not exist" << endl;
            return;
        }
        if (!job->isStoppedJob()) {
            cerr << "smash error: bg: job-id " << getArgs()[1] << " is already running in the background" << endl;
            return;
        }
    }
    cout << job->getCmd() << " : " << job->getProcessId() << endl;
    if (kill(job->getProcessId(), SIGCONT) == FAILURE)
        SYS_CALL_ERROR_MESSAGE("kill");
    job->setNotStopped();
}

void QuitCommand::execute() {
    if (getArgsCount() >= 1 && string(getArgs()[1]).compare("kill"))
        jobs->killAllJobs();
    exit(0);
}

void ExternalCommand::execute() {
    cout<< "hello all"<<endl;
    pid_t pid = fork();
    if (pid == FAILURE)
        SYS_CALL_ERROR_MESSAGE("fork");
    if (pid == 0) {

        setpgrp();
        SmallShell &smash = SmallShell::getInstance();
        if (_isBackgroundCommand(getCmdLine().c_str()))
            smash.getJobList()->addJob(this, false);
        else
            smash.setJobToForeground(this);
        char *argv[4];
        char new_cmd_line[COMMAND_ARGS_MAX_LENGTH];
        strcpy(new_cmd_line, getCmdLine().c_str());
        argv[2] = new_cmd_line;
        argv[3] = nullptr;
        argv[0] = (char *) malloc(10);
        strcpy(argv[0], "/bin/bash");
        argv[1] = (char *) malloc(3);
        strcpy(argv[1], "-c");
        if (execv(argv[0], argv) == FAILURE)
            SYS_CALL_ERROR_MESSAGE("execv");
    } else {
        if (!_isBackgroundCommand(getCmdLine().c_str()))
            if (waitpid(pid, nullptr, WUNTRACED) == FAILURE)
                SYS_CALL_ERROR_MESSAGE("waitpid");
    }
}
