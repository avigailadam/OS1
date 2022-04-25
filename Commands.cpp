#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <time.h>
#include <utime.h>


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
    cout << "smash error: " << getName() << ":" << message << endl; \
    return;} while(0)


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
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s;) {
        args[i] = (char *) malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}


bool _isBackgroundComamnd(const char *cmd_line) {
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

SmallShell::SmallShell() {
    prompt = "smash";
}

SmallShell::~SmallShell() {
    delete plastPwd;
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command *SmallShell::CreateCommand(const char *cmd_line) {

    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));


    if (firstWord.compare("pwd") == 0) {
        return new GetCurrDirCommand(cmd_line);
    } else if (firstWord.compare("showpid") == 0) {
        return new ShowPidCommand(cmd_line);
    } else if (firstWord.compare("chprompt") == 0) {
        return new ChangePromptCommand(cmd_line, &prompt);
    } else if (firstWord.compare("cd") == 0) {
        return new ChangeDirCommand(cmd_line, plastPwd);
    } else if (firstWord.compare("kill") == 0) {
        return new KillCommand(cmd_line, jobs);
    } else if (firstWord.compare("jobs") == 0) {
        return new JobsCommand(cmd_line, jobs);
    } else if (firstWord.compare("fg") == 0) {
        return new ForegroundCommand(cmd_line, jobs);
    } else if (firstWord.compare("bg") == 0) {
        return new BackgroundCommand(cmd_line, jobs);
    } else {
        return new ExternalCommand(cmd_line);
    }

    return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
    // TODO: Add your implementation here
    // for example:
    // Command* cmd = CreateCommand(cmd_line);
    // cmd->execute();
    // Please note that you must fork smash process for some commands (e.g., external commands....)
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
    cout << "smash pid is" << smash.getPid() << endl;
}

void ChangeDirCommand::execute() {
    if (getArgsCount() > 2)
        PRINT_SMASH_ERROR_AND_RETURN("too many arguments");
    string path = string(getArgs()[1]);
    if (path.compare("-") == 0) {
        if (plastPwd == nullptr)
            PRINT_SMASH_ERROR_AND_RETURN("OLDPWD not set");
        char *tmp = get_current_dir_name();
        chdir(*plastPwd);
        delete plastPwd;
        *plastPwd = tmp;
        return;
    }
    if (path.compare("\0") == 0)
        path = get_current_dir_name();
    *plastPwd = get_current_dir_name();
    chdir(path.c_str());

}

void KillCommand::execute() {
    if (getArgsCount() != 3 || stoi(string(getArgs()[1])) >= 0)
        PRINT_SMASH_ERROR_AND_RETURN("invalid arguments");
    int sigNum = stoi(string(getArgs()[1]));
    sigNum *= (-1);
    int jobId = stoi(string(getArgs()[2]));
    if (!jobs->jobExist(jobId))
        PRINT_SMASH_ERROR_AND_RETURN("job-id" + to_string(jobId) + "does not exist");
    int pid = jobs->getJobById(jobId)->getProcessId();
    if (kill(pid, sigNum) == 0) {
        cout << "signal number " << sigNum << " was sent to pid " << pid << endl;
        return;
    }
    //handle perror

}

void JobsCommand::execute() {
    jobs->removeFinishedJobs();
    jobs->printJobsList();
}

void ForegroundCommand::execute() {
    if (getArgsCount() > 2 || getArgsCount() == 0)
        PRINT_SMASH_ERROR_AND_RETURN("invalid arguments");
    if(getArgsCount()==1){

    }
    int jobId = stoi(string(getArgs()[1]));

}