#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
    cout << "smash: got ctrl-Z" << endl;
    SmallShell &smash = SmallShell::getInstance();
    Command *fg = smash.getForegroundCommand();
    if (fg == nullptr)
        return;
    smash.getJobList()->addJob(fg, fg->getPid(), true);
    if (kill(fg->getPid(), SIGSTOP) == FAILURE)
        SYS_CALL_ERROR_MESSAGE("stop");
    cout << "smash: process " << fg->getPid() << " was stopped" << endl;
}

void ctrlCHandler(int sig_num) {
    cout << "smash: got ctrl-C" << endl;
    SmallShell &smash = SmallShell::getInstance();
    Command *fg = smash.getForegroundCommand();
    if (fg == nullptr)
        return;
    if (kill(fg->getPid(), SIGKILL) == FAILURE)
        SYS_CALL_ERROR_MESSAGE("kill");
    cout << "smash: process " << fg->getPid() << " was killed" << endl;
}

void alarmHandler(int sig_num) {
    // TODO: Add your implementation
}

