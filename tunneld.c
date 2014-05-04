
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdint.h>
#include <fcntl.h>
#include <err.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/param.h>
#include <libutil.h>
#include <syslog.h>
#include <stdarg.h>
#include <math.h>
#include <pwd.h>
#include <libconfig.h>
#include <string.h>
#include <stdlib.h> 

#include "arg_parse.h"

#define DAEMON_USER "tunnel"
#define MAX_ARG_SIZE 15

typedef struct command_t {
    int pid;
    int timeout;
    char * name;
    char ** args;
} command_t;

int command_len;

command_t * commands;
int cleanup();
void usage();
struct pidfh *pfh;
void signal_handler(int sig);
int setuids(const char * username);
int connection(const char * username, char ** vars, int stuid);

int
main (int argc, char ** argv)
{
    int cpid;
    int i;
    int status;
    int daemonize = 0;
    int command_len = 0;
    const char * username = "tunnel";
    commands = (command_t *) malloc (sizeof (command_t) * MAX_ARG_SIZE);

    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-c") == 0) 
        {
            int parts = arg_parse(argv[++i], &commands[command_len++].args);
        }
        else if (strcmp(argv[i], "-d") == 0)
        {
            daemonize = 1;
        }
        else
        {
            usage();
            return (EXIT_FAILURE);
        }
    }

    pid_t otherpid;

    if (daemonize)
    {
        signal(SIGTERM, signal_handler);
        pfh = pidfile_open(NULL, 0600, &otherpid);
        if (pfh == NULL) {
            if (errno == EEXIST) {
                errx(EXIT_FAILURE, "Daemon already running, pid: %jd.", (intmax_t) otherpid);
            }
            warn("Cannot open or create pidfile");
            exit(EXIT_FAILURE);
        }

        if (daemon(0, 0) == -1) {
            warn("Cannot daemonize");
            pidfile_remove(pfh);
            exit(EXIT_FAILURE);
        }

        pidfile_write(pfh);
    }

    syslog(LOG_DEBUG, "Spawning Connections. %i", command_len);
    

    for (i = 0; i < command_len; i++)
    {
        cpid = fork();
        
        // Error State
        if (cpid < 0)
        {
            syslog(LOG_CRIT, "Failed to fork!");
            syslog(LOG_DEBUG, "Failed to fork!");
            break;
        }

        // Child
        if (cpid == 0)
        {        
            syslog(LOG_DEBUG, "Beginning Connection");
            signal(SIGCHLD, SIG_IGN);
            connection(DAEMON_USER, (char **) commands[i].args, daemonize);
            syslog(LOG_DEBUG, "Failed to Exec");
            exit(127);
        }
        
        commands[i].pid = cpid;

    }

    syslog(LOG_DEBUG, "Connections Spawned.");

    while (1)
    {

        if ((cpid = wait(&status)) < 0) {
            perror("wait");
            break;
        }

        if (WIFEXITED(status))
        {
            syslog(LOG_DEBUG, "Exited with %i", WEXITSTATUS(status));
        }
        else
        {
            syslog(LOG_DEBUG, "Exited with 127");
        }

        for (i = 0; i < command_len; i++) 
        {
            if ( cpid == commands[i].pid ) 
            {
                cpid = fork();
                
                // Error State
                if (cpid < 0)
                {
                    perror("Fork!!");
                    syslog(LOG_CRIT, "Failed to fork!");
                    break;
                }

                // Child
                if (cpid == 0)
                {        
                    signal(SIGTERM, signal_handler);
                    syslog(LOG_DEBUG, "Beginning Connection");
                    connection(DAEMON_USER, (char **) commands[i].args, daemonize);
                    syslog(LOG_DEBUG, "Failed to Exec");
                    exit(127);
                }

                commands[i].pid = cpid;
                sleep(pow(2, commands[i].timeout));
                commands[i].timeout++;
                break;                
            } 
        }
    }
    
    syslog(LOG_ERR, "Daemon Exiting1");
    cleanup();
    syslog(LOG_DEBUG, "Daemon Exiting2");
    pidfile_remove(pfh);
    free (commands);
    return 0;
}

void usage()
{
    printf("Usage: tunneld -c COMM [-d] \n");
    printf(" -c COMM      specify a command to run\n");
    printf(" -d           daemonize\n");
}

int cleanup()
{
    int i = 0;
    for (i = 0; i < command_len; i++) {
        kill((pid_t)commands[i].pid, SIGTERM);
    }
    return 0;
}


int connection(const char * username, char ** vars, int stuid)
{

    if (stuid){
        if (setuids(username) < 0) {
            return 1;
        }
    }
    
    int devNull = open("/dev/null", O_WRONLY);

    //if (dup2(devNull, 2) < 0) {
    //    perror("dup2");
    //    return 1;
    //}

    if (dup2(devNull, 1) < 0) {
        perror("dup2");
        return 1;
    }

    close(devNull);
    execv(vars[0], &vars[0]);

    perror("Fork");
    return 1;
}

int setuids(const char * username)
{
    struct passwd * pw;

    pw = getpwnam(username);
    
    //printf("%s, %i, %i\n", pw->pw_name, (int)pw->pw_uid, (int)pw->pw_gid);

    if (setgid(pw->pw_gid) < 0) {
        perror("Setgid");
        return -1;
    }

    if (setuid(pw->pw_uid) < 0) {
        perror("Setuid");
        return -1;
    }

    return 0;
}

void
signal_handler (int sig)
{
    syslog(LOG_ERR, "Daemon Exiting. Recieved: %i", sig);
    syslog(LOG_DEBUG, "Daemon Exiting. Recieved: %i", sig);
    pidfile_remove(pfh);
    cleanup();
    exit(127);
}
