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

bool _isRedirectionCommand(const char *cmd_line) {
    const string str(cmd_line);
    return str.find(">") != string::npos || str.find(">>") != string::npos;
}

bool _isPipeCommand(const char *cmd_line) {
    const string str(cmd_line);
    return str.find("|") != string::npos || str.find("|&") != string::npos;
}

SmallShell::SmallShell() : plastPwd(""), fgJobId(0) {
    prompt = "smash";
    jobs = new JobsList();
    currForegroundCommand = nullptr;
    pid = getpid();
}

SmallShell::~SmallShell() {
    delete jobs;
    delete currForegroundCommand;
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command *SmallShell::CreateCommand(const char *cmd_line) {

    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
    size_t size = string(cmd_line).find_last_not_of(WHITESPACE) + 2;
    char *built_in_cmd_line = new char[size];
    memcpy(built_in_cmd_line, cmd_line, size);
    if (_isBackgroundCommand(built_in_cmd_line))
        _removeBackgroundSign(built_in_cmd_line);
    if (_isPipeCommand(cmd_line)) {
        return new PipeCommand(cmd_line);
    } else if (_isRedirectionCommand(cmd_line)) {
        return new RedirectionCommand(cmd_line);
    }else if (firstWord.compare("pwd") == 0) {
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
    } else if (firstWord.compare("tail") == 0) {
        return new TailCommand(built_in_cmd_line);
    } else if (firstWord.compare("touch") == 0) {
        return new TouchCommand(built_in_cmd_line);
    } else{
        return new ExternalCommand(cmd_line);
    }

    return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
    Command *cmd = CreateCommand(cmd_line);
    if (dynamic_cast<BuiltInCommand *>(cmd))
        setJobToForeground(cmd);
    else if (!_isBackgroundCommand(cmd_line))
        setJobToForeground(cmd);
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
            cerr << "smash error: fg: job-id " << jobId << " does not exists" << endl;
            return;
        }
    }
    pid_t pid = currJob->getProcessId();
    SmallShell &smash = SmallShell::getInstance();
    smash.setJobToForeground(currJob->getCommand(), currJob->getJobId());
    cout << currJob->getCmdLine() << " : " << pid << endl;
    if (killpg(pid, SIGCONT) == FAILURE)
        SYS_CALL_ERROR_MESSAGE("kill");
    if (waitpid(pid, nullptr, WUNTRACED) == FAILURE)
        SYS_CALL_ERROR_MESSAGE("waitpid");
    assert(currJob);
    assert(jobs);
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
    cout << job->getCmdLine() << " : " << job->getProcessId() << endl;
    if (kill(job->getProcessId(), SIGCONT) == FAILURE)
        SYS_CALL_ERROR_MESSAGE("kill");
    job->setNotStopped();
}

void QuitCommand::execute() {
    if (getArgsCount() > 1 && string(getArgs()[1]).compare("kill"))
        jobs->killAllJobs();
    exit(0);
}

void ExternalCommand::execute() {
    pid_t pid = fork();
    if (pid == FAILURE)
        SYS_CALL_ERROR_MESSAGE("fork");
    if (pid == 0) {
        setpgrp();
        setPid();
        char *argv[4];
        char new_cmd_line[COMMAND_ARGS_MAX_LENGTH];
        strcpy(new_cmd_line, getCmdLineAsString().c_str());
        _removeBackgroundSign(new_cmd_line);
        argv[2] = new_cmd_line;
        argv[3] = nullptr;
        argv[0] = (char *) malloc(10);
        strcpy(argv[0], "/bin/bash");
        argv[1] = (char *) malloc(3);
        strcpy(argv[1], "-c");
        if (execv(argv[0], argv) == FAILURE)
            SYS_CALL_ERROR_MESSAGE("execv");
        free(argv[0]);
        free(argv[1]);
    } else {
        if (_isBackgroundCommand(getCmdLineAsString().c_str())) {
            SmallShell &smash = SmallShell::getInstance();
            int jobIdToSet = smash.getJobList()->getJobIdToSet();
            smash.getJobList()->addJob(this, jobIdToSet, pid, false);
        } else {
            SmallShell &smash = SmallShell::getInstance();
            smash.setForegroundPidFromFather(pid);
            smash.setJobToForeground(this);
            if (waitpid(pid, nullptr, WUNTRACED) == FAILURE)
                SYS_CALL_ERROR_MESSAGE("waitpid");
        }
    }
}

bool isValidNumber(const string &str) {
    for (char const &c : str) {
        if (std::isdigit(c) == 0) return false;
    }
    return true;
}


void TailCommand::execute() {
    if (getArgsCount() > 3 || getArgsCount() <= 1)
        PRINT_SMASH_ERROR_AND_RETURN("invalid arguments");
    int N = 10;
    string path = string(getArgs()[1]);
    if (getArgsCount() == 3) {
        if (path[0] != '-')
            PRINT_SMASH_ERROR_AND_RETURN("invalid arguments");
        path.erase(0, 1);
        if (!(isValidNumber(path)))
            PRINT_SMASH_ERROR_AND_RETURN("invalid arguments");
        N = stoi(path);
        path = string(getArgs()[2]);
    }
    ifstream file;
    file.open(path, ifstream::in);
    if (!file) {
        perror("smash error: open failed");
        return;
    }
    string line;
    for (int i = 0; i < N && getline(file, line); i++) {
        cout << line << endl;
    }
    file.close();


}

string string_before_char(string orig, string c) {
    return _trim(orig.substr(0, orig.find(c)));
}

string string_after_char(string orig, string c) {
    return _trim(orig.substr(orig.find(c) + c.size()));
}


void TouchCommand::execute() {
    if (getArgsCount() != 3)
        PRINT_SMASH_ERROR_AND_RETURN("invalid arguments");
    string timeToSet= string(getArgs()[2]);
    struct tm time;
    time.tm_sec = stoi(string_before_char(timeToSet, ":"));
    timeToSet = string_after_char(timeToSet, ":");
    time.tm_min = stoi(string_before_char(timeToSet,":"));
    timeToSet = string_after_char(timeToSet, ":");
    time.tm_hour = stoi(string_before_char(timeToSet,":"))-1;
    timeToSet = string_after_char(timeToSet, ":");
    time.tm_mday = stoi(string_before_char(timeToSet,":"));
    timeToSet = string_after_char(timeToSet, ":");
    time.tm_mon = stoi(string_before_char(timeToSet,":"))-1;
    time.tm_year = stoi(string_after_char(timeToSet, ":"))-1900;
    time_t new_time = mktime(&time);
    struct utimbuf utimeToSet;
    utimeToSet.modtime = new_time;

    utimeToSet.actime = new_time;
    if (utime(getArgs()[1], &utimeToSet) == FAILURE){
        SYS_CALL_ERROR_MESSAGE("utime");
    }

}



void PipeCommand::execute() {
    int pipeline[2];
    if (pipe(pipeline) == FAILURE)
        SYS_CALL_ERROR_MESSAGE("pipe");
    string c = getCmdLineAsString().find("|&") == string::npos ? "|" : "|&";
    string cmd_line1 = string_before_char(getCmdLineAsString(), c);
    string cmd_line2 = string_after_char(getCmdLineAsString(), c);
    SmallShell &smash = SmallShell::getInstance();
    pid_t pid1 = fork();
    if (pid1 == FAILURE)
        SYS_CALL_ERROR_MESSAGE("fork");
    if (pid1 == 0) {
        setpgrp();
        setPid();
        close(c == "|" ? 1 : 2);
        close(pipeline[0]);
        dup2(pipeline[1], c == "|" ? 1 : 2);
        smash.executeCommand(cmd_line1.c_str());
        close(pipeline[1]);
        exit(0);
    }
    pid_t pid2 = fork();
    if (pid2 == FAILURE)
        SYS_CALL_ERROR_MESSAGE("fork");
    if (pid2 == 0) {
        setpgrp();
        setPid();
        close(c == "|" ? 0 : 2);
        close(pipeline[1]);
        dup2(pipeline[0], c == "|" ? 0 : 2);
        close(pipeline[0]);
        smash.executeCommand(cmd_line2.c_str());
        exit(0);
    }
    close(pipeline[0]);
    close(pipeline[1]);
    wait(nullptr);
    wait(nullptr);
}

void RedirectionCommand::execute() {
    string c = getCmdLineAsString().find(">>") == string::npos ? ">" : ">>";
    string cmd = string_before_char(getCmdLineAsString(), c);
    string path = string_after_char(getCmdLineAsString(), c);
    pid_t pid = fork();
    if (pid == FAILURE)
        SYS_CALL_ERROR_MESSAGE("fork");
    if (pid == 0) {
        setpgrp();
        setPid();
        int new_fd = open(path.c_str(), O_WRONLY | O_CREAT | (c == ">" ? O_TRUNC : O_APPEND), 0777);
        if (new_fd == FAILURE)
            SYS_CALL_ERROR_MESSAGE("open");
        close(1);
        dup2(new_fd, 1);
        SmallShell &smash = SmallShell::getInstance();
        smash.executeCommand(cmd.c_str());
        close(new_fd);
        exit(0);
    }
}
