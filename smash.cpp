#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"

int main(int argc, char* argv[]) {
    if(signal(SIGTSTP , ctrlZHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if(signal(SIGINT , ctrlCHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }

    //TODO: setup sig alarm handler

    SmallShell& smash = SmallShell::getInstance();
    while(true)
        {
            std::cout << smash.getName() + ">";
            std::string cmd_line;
            std::getline(std::cin, cmd_line);
            smash.executeCommand(cmd_line.c_str());
<<<<<<< Updated upstream
            smash.jobs.removeFinishedJobs();
            //assert(smash.jobs.getFg() == nullptr);
=======
>>>>>>> Stashed changes
        }
    return 0;
}
/*
int main(){
    int fd[2];
    pipe(fd);
    if (fork() == 0) {
    // first child
        dup2(fd[1],1);
        close(fd[0]);
        close(fd[1]);
        char* args[2];
        args[0] = "/bin/ls";
        args[1] = NULL;
        execv(args[0],args );
    }
    if (fork() == 0) {
    // second child
        dup2(fd[0],0);
        close(fd[0]);
        close(fd[1]);
        char* args[2];
        args[0] = "/bin/more";
        args[1] = NULL;
        execv(args[0],args );
    }
    close(fd[0]);
    close(fd[1]);
}
*/
