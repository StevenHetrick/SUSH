#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <signal.h>
#include <fcntl.h>

#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64
#define MAX_LIM 1024

struct rusage usage;
int tokCount = 0;

typedef struct
{
    char tokens[MAX_LIM]; //tokens stored here
    int tok_count;        //keeps count of num of tokens
    char PS1[MAX_LIM];    //ps1 stored here
} cmd_t;

typedef struct
{
    char name[MAX_LIM];  //variable name
    char value[MAX_LIM]; //variable value
} env_t;                 //structure for environment variables

enum states
{
    START,
    END,
    FIND,
    PARA,
    CHOOSE,
    SLASH,
    OUTCAT,
    IN,
    PIPE,
    WORD,
    EQ
}; //states for state machine

env_t env[MAX_LIM];

int check(cmd_t *cmd)
{
    if (cmd->tok_count == 1)
    {
        if (strcmp(cmd[0].tokens, "ls") == 0 || strcmp(cmd[0].tokens, "exit") == 0)
        {
            return (1);
        }
        else
        {
            printf("Error: Command Not Valid\n"); //this can be at the end because everything will return its own
            for (int i = 0; i < cmd->tok_count; i++)
            {
                for (int j = 0; j < sizeof(cmd[i].tokens); j++)
                    cmd[i].tokens[j] = '\0';
            }
            return (0);
        }
    }
}

//this function splits up the string entered into tokens
int token(cmd_t *cmd, const char *line)
{
    int len = strlen(line); //initalises local vars
    enum states state = FIND;
    int pos = 0;
    int x = 0;
    char ch;
    int start = pos;
    while (pos <= len)
    { //loops till end of commnd entered
        ch = line[pos];

        //	printf("STATE: %d pos: %d ch = %c\n", state, pos, ch);
        switch (state)
        {
        case FIND: //finds begining of string
            if (ch == ' ')
            {
                start = pos;
            }
            else
            {
                state = START;
                start = pos;
            }
            break;
        case START: //takes first token
            if (ch == ' ')
            {
                strncpy(cmd[x].tokens, line + start, pos - start);
                start = pos;
                state = CHOOSE;
                x++;
            }
            if (ch == '\n')
            {
                strncpy(cmd[x].tokens, line + start, pos - start);
                pos--;
                state = END;
                x++;
            }
            if (ch == '/')
            {
                strncpy(cmd[x].tokens, line + start, pos - start);
                start = pos;
                state = SLASH;
                x++;
            }
            if (ch == '"')
            {
                strncpy(cmd[x].tokens, line + start, pos - start);
                start = pos + 1;
                state = PARA;
                x++;
            }
            if (ch == '=')
            {
                strncpy(cmd[x].tokens, line + start, pos - start);
                state = EQ;
                x++;
            }
            if (ch == '>')
            {
                strncpy(cmd[x].tokens, line + start, pos - start);
                start = pos;
                state = OUTCAT;
                x++;
            }
            if (ch == '<')
            {
                strncpy(cmd[x].tokens, line + start, pos - start);
                start = pos;
                state = IN;
                x++;
            }
            if (ch == '|')
            {
                strncpy(cmd[x].tokens, line + start, pos - start);
                start = pos;
                state = PIPE;
                x++;
            }
            break;
        case CHOOSE: //handles next non blank character if the previous one was blank
            if (ch == ' ')
            {
                start = pos;
            }
            else
            {
                if (ch == '\n')
                {
                    state = END;
                }
                else if (ch == '/')
                {
                    start = pos;
                    state = SLASH;
                }
                else if (ch == '=')
                {
                    start = pos;
                    state = EQ;
                }
                else if (ch == '"')
                {
                    start = pos + 1;
                    state = PARA;
                }
                else if (ch == '>')
                {
                    start = pos;
                    state = OUTCAT;
                }
                else if (ch == '<')
                {
                    start = pos;
                    state = IN;
                }
                else if (ch == '|')
                {
                    start = pos;
                    state = PIPE;
                }
                else
                {
                    start = pos;
                    state = WORD;
                }
            }
            break;
        case WORD: //handles if there is another "word" entered
            if (ch == ' ')
            {
                strncpy(cmd[x].tokens, line + start, pos - start);
                start = pos;
                state = CHOOSE;
                x++;
            }
            if (ch == '\n')
            {
                strncpy(cmd[x].tokens, line + start, pos - start);
                pos--;
                state = END;
                x++;
            }
            if (ch == '"')
            {
                strncpy(cmd[x].tokens, line + start, pos - start);
                start = pos + 1;
                state = PARA;
                x++;
            }
            if (ch == '>')
            {
                strncpy(cmd[x].tokens, line + start, pos - start);
                start = pos;
                state = OUTCAT;
                x++;
            }
            if (ch == '<')
            {
                strncpy(cmd[x].tokens, line + start, pos - start);
                start = pos;
                state = IN;
                x++;
            }
            if (ch == '|')
            {
                strncpy(cmd[x].tokens, line + start, pos - start);
                start = pos;
                state = PIPE;
                x++;
            }
            if (ch == '/')
            {
                strncpy(cmd[x].tokens, line + start, pos - start);
                start = pos;
                state = SLASH;
                x++;
            }
            break;
        case EQ:
            if (ch == '"')
            {
                start = pos + 1;
                state = PARA;
            }
            if (ch == ' ')
            {
                start = pos;
                state = CHOOSE;
            }
            break;
        case SLASH: //handles if a "/" was entered
            if (ch == ' ')
            {
                strncpy(cmd[x].tokens, line + start, pos - start);
                start = pos;
                state = CHOOSE;
                x++;
            }
            if (ch == '\n')
            {
                strncpy(cmd[x].tokens, line + start, pos - start);
                pos--;
                state = END;
                x++;
            }
            if (ch == '"')
            {
                strncpy(cmd[x].tokens, line + start, pos - start);
                start = pos + 1;
                state = PARA;
                x++;
            }
            if (ch == '>')
            {
                strncpy(cmd[x].tokens, line + start, pos - start);
                start = pos;
                state = OUTCAT;
                x++;
            }
            if (ch == '<')
            {
                strncpy(cmd[x].tokens, line + start, pos - start);
                start = pos;
                state = IN;
                x++;
            }
            if (ch == '|')
            {
                strncpy(cmd[x].tokens, line + start, pos - start);
                start = pos;
                state = PIPE;
                x++;
            }
            break;
        case IN: //handles if a "<" was entered
            strncpy(cmd[x].tokens, line + start, pos - start);

            if (ch == ' ')
            {
                start = pos;
                state = CHOOSE;
                x++;
            }
            else if (ch == '\n')
            {
                pos--;
                state = END;
                x++;
            }
            else if (ch == '"')
            {
                start = pos + 1;
                state = PARA;
                x++;
            }
            else if (ch == '>')
            {
                start = pos;
                state = OUTCAT;
                x++;
            }
            else if (ch == '/')
            {
                start = pos;
                state = SLASH;
                x++;
            }
            else if (ch == '<')
            {
                start = pos;
                state = IN;
                x++;
            }
            else if (ch == '|')
            {
                start = pos;
                state = PIPE;
                x++;
            }
            else
            {
                start = pos;
                state = WORD;
                x++;
            }
            break;
        case PIPE: //handles if a "|" was entered
            strncpy(cmd[x].tokens, line + start, pos - start);
            if (ch == ' ')
            {
                start = pos;
                state = CHOOSE;
                x++;
            }
            else if (ch == '\n')
            {
                pos--;
                state = END;
                x++;
            }
            else if (ch == '"')
            {
                start = pos + 1;
                state = PARA;
                x++;
            }
            else if (ch == '>')
            {
                start = pos;
                state = OUTCAT;
                x++;
            }
            else if (ch == '/')
            {
                start = pos;
                state = SLASH;
                x++;
            }
            else if (ch == '<')
            {
                start = pos;
                state = IN;
                x++;
            }
            else if (ch == '|')
            {
                start = pos;
                state = PIPE;
                x++;
            }
            else
            {
                start = pos;
                state = WORD;
                x++;
            }
            break;
        case OUTCAT: //handles if a ">" or ">>" was entered
            if (ch == '>')
            {
                strncpy(cmd[x].tokens, line + start, pos + 1 - start);
                start = pos;
                state = CHOOSE;
                x++;
            }
            else
            {
                strncpy(cmd[x].tokens, line + start, pos - start);
                if (ch == ' ')
                {
                    strncpy(cmd[x].tokens, line + start, pos - start);
                    start = pos;
                    state = CHOOSE;
                    x++;
                }
                else if (ch == '\n')
                {
                    strncpy(cmd[x].tokens, line + start, pos - start);
                    pos--;
                    state = END;
                    x++;
                }
                else if (ch == '"')
                {
                    strncpy(cmd[x].tokens, line + start, pos - start);
                    start = pos + 1;
                    state = PARA;
                    x++;
                }
                else if (ch == '/')
                {
                    strncpy(cmd[x].tokens, line + start, pos - start);
                    start = pos;
                    state = SLASH;
                    x++;
                }
                else if (ch == '<')
                {
                    strncpy(cmd[x].tokens, line + start, pos - start);
                    start = pos;
                    state = IN;
                    x++;
                }
                else if (ch == '|')
                {
                    strncpy(cmd[x].tokens, line + start, pos - start);
                    start = pos;
                    state = PIPE;
                    x++;
                }
                else
                {
                    strncpy(cmd[x].tokens, line + start, pos - start);
                    start = pos;
                    state = WORD;
                    x++;
                }
            }
            break;
        case PARA: //handles if a "some text" was entered
            if (ch != '"')
            {
            }
            else
            {
                strncpy(cmd[x].tokens, line + start, pos - start);
                pos++;
                ch = line[pos];

                if (ch == ' ')
                {
                    start = pos;
                    state = CHOOSE;
                    x++;
                }
                else if (ch == '\n')
                {
                    state = END;
                    x++;
                }
                else if (ch == '/')
                {
                    start = pos;
                    state = SLASH;
                    x++;
                }
                else if (ch == '>')
                {
                    start = pos;
                    state = OUTCAT;
                    x++;
                }
                else if (ch == '<')
                {
                    start = pos;
                    state = IN;
                    x++;
                }
                else if (ch == '|')
                {
                    start = pos;
                    state = PIPE;
                    x++;
                }
                else
                {
                    start = pos;
                    state = WORD;
                    x++;
                }
            }
            break;
        case END:
            cmd->tok_count = x; //keeps count of tokens
            return (-1);

            break;
        default:
            printf("illegal state");
            goto error;
        }
        pos++;
    }

error: //handles if there was an error in the state machine
    fprintf(stderr, "Error - crash at %d\n", pos);
    return -1;
}

// fork the process so the child can execute the command
void run(const char *execName, char *const argv[])
{
    int rc;
    int pipes[2];
    rc = pipe(pipes);
    if (rc < 0)
    {
        perror("could not create pipes");
        return;
    }

/////////////////////////////////////////

    int hasPipe = 0;
    int i;
    for(i = 0; i < tokCount; i++) {
        if (!strcmp(argv[i], "|"))
        {
            hasPipe = 1;
            break;
        }
    }
//    printf("i = %d\n", i);
//    printf("tokCount = %d\n", tokCount);
    char *argPipe1[i + 1];
    char *argPipe2[tokCount - i];

    if (hasPipe)
    {
        for (int j = 0; j < tokCount - i - 1; j++)
        {
            argPipe1[j] = strdup(argv[j]);
            argPipe2[j] = strdup(argv[j + i + 1]);
        }
        argPipe1[i] = NULL;
        argPipe2[tokCount - i - 1] = NULL;
        //parent arg Pipe 1
        //child arg Pipe 2
        tokCount/=2;
    }

/////////////////////////////////////////////

    pid_t pid = fork();

    if (pid == 0)
    { //child
        close(pipes[1]);

        signal(SIGINT, SIG_DFL);
        dup(pipes[1]);

        // char* pipeIn = "t";
        // read(pipes[0], &pipeIn, sizeof(pipeIn));
        // printf("%s doop\n", pipeIn);

        int i;
        int out = -1;
        int in = -1;

        for(i = 0; i < tokCount; i++) {
            if (!strcmp(argv[i], ">")) {
                FILE *fp = fopen(argv[i+1], "wr");
                out = open(argv[i + 1], O_WRONLY | O_CREAT);
                tokCount -= 2;
                break;
            }
            else if (!strcmp(argv[i], ">>")) {
                FILE *fp = fopen(argv[i + 1], "a");
                out = open(argv[i + 1], O_APPEND | O_WRONLY | O_CREAT);
                tokCount -= 2;
                break;
            }
            else if (!strcmp(argv[i], "<")) {
                FILE *fp = fopen(argv[i + 1], "r");
                in = open(argv[i + 1], O_RDONLY);
                break;

            } 
            
        }

        //if has a pipe recursion
        if (out != -1) {
            dup2(out, 1);
            char *arg[tokCount + 1];
            for(int j = 0; j < tokCount; j++) {
                arg[j] = strdup(argv[j]);
            }
            arg[tokCount] = NULL;
            close(out);
            run(arg[0], arg);
        } else if (in != -1) {
            dup2(in, 0);
            char *arg[tokCount + 1];
            int k = 0;
            for (int j = 0; j < tokCount; j++)
            {
                if(!strcmp(argv[j], "<")){
                    continue;
                }
                arg[k] = strdup(argv[j]);
                k++;
            }
            tokCount--;
            arg[tokCount] = NULL;
            close(in);
            run(arg[0], arg);

        } else if (hasPipe) {

            
            dup2(pipes[0], 0);
            dup2(pipes[1], 1);
            
            run(argPipe2[0], argPipe2);

            dup(0);
            dup(1);

        } else
        {
            execvp(argv[0], argv); //execute command

            perror("could not execute");
            exit(-1);
        }

        exit(-1);
    
    }
    else
    { //parent
        close(pipes[0]);

        if (hasPipe)
        {
            pid_t pid = fork();
            if(pid == 0) {
                dup2(pipes[1], 1);
                run(argPipe1[0], argPipe1);
                exit(-1);
            } else {
                int status;
                wait4(pid, &status, 0, &usage);
            }    
        }

        int status;
        wait4(pid, &status, 0, &usage);  //get child resource information on death
        //waitpid(pid, &status, 0); //wait for child to die
    }
}

void clear(cmd_t *cmd)
{
    for (int i = 0; i < cmd->tok_count; i++)
    {
        for (int j = 0; j < sizeof(cmd[i].tokens); j++)
            cmd[i].tokens[j] = '\0';
    }
}

void accnt()
{
    
    getrusage(RUSAGE_CHILDREN, &usage);
    printf("\nCPU time: %ld.%06ld sec user, %ld.%06ld sec system\n",
           usage.ru_utime.tv_sec, usage.ru_utime.tv_usec,
           usage.ru_stime.tv_sec, usage.ru_stime.tv_usec);
    printf("Max resident set size: %ld\n", usage.ru_maxrss);
    printf("Integral shared memory size: %ld\n", usage.ru_ixrss);
    printf("Integral unshared data size: %ld\n", usage.ru_idrss);
    printf("Integral unshared stack size: %ld\n", usage.ru_isrss);
    printf("Page reclaims (soft page faults): %ld\n", usage.ru_minflt);
    printf("Page faults (hard page faults): %ld\n", usage.ru_majflt);
    printf("Swaps: %ld\n", usage.ru_nswap);
    printf("Block input operations: %ld\n", usage.ru_inblock);
    printf("Block output operations: %ld\n", usage.ru_oublock);
    printf("IPC messages sent: %ld\n", usage.ru_msgsnd);
    printf("IPC messages received: %ld\n", usage.ru_msgrcv);
    printf("Signals received: %ld\n", usage.ru_nsignals);
    printf("Voluntary context switches: %ld\n", usage.ru_nvcsw);
    printf("Involuntary context switches: %ld\n", usage.ru_nivcsw);
}

int commands(cmd_t *cmd)
{

    if (!strcmp(cmd[0].tokens, "setenv"))
    { //set var "cmd[1].tokens" = "cmd[2].tokens"
        setenv(cmd[1].tokens, cmd[2].tokens, 0);

        return (0);
    }
    else if (!strcmp(cmd[0].tokens, "unsetenv"))
    { //delete var "cmd[1].tokens"
        unsetenv(cmd[1].tokens);
        
        return (0);
    }
    else if (!strcmp(cmd[0].tokens, "cd"))
    { //cd dir
        chdir(cmd[1].tokens);
    }
    else if (!strcmp(cmd[0].tokens, "pwd"))
    { //print current directory
        char buff[MAX_LIM];
        getcwd(buff, MAX_LIM);
        printf("%s\n", buff);
    }
    else if (!strcmp(cmd[0].tokens, "accnt"))
    { //process accounting
        accnt();
        return 0;
    }
    else if (cmd[0].tokens[0] == '.')
    { //tokenizer doesn't handle when user tries to run a file
        strcat(cmd[0].tokens, cmd[1].tokens);
        char *args[] = {cmd[0].tokens, NULL};
        run(args[0], args);
    }
    else
    { //if not an internal command use default commands
        char *args[cmd->tok_count + 1];

        for (int i = 0; i < cmd->tok_count; i++)
        {
            args[i] = strdup(cmd[i].tokens);
        }

        args[cmd->tok_count] = NULL;
        tokCount = cmd->tok_count;
        run(cmd[0].tokens, args);
    }
}

int main(int argc, char **argv)
{
    signal(SIGINT, SIG_IGN);
    signal(SIGUSR1, accnt);
    cmd_t cmd[MAX_LIM]; //initializes struct
    char file[MAX_LIM];
    strcat(strcpy(file, getenv("HOME")), "/.sushrc"); //finds home directory
    FILE *RC;
    char line[MAX_LIM]; //stores entered string
    if (access(file, F_OK) != -1)
    {
        RC = fopen(file, "r"); //opens .sushrc
        while (fgets(line, sizeof(line), RC) != NULL)
        {
            clear(cmd);
            token(cmd, line);
            if (!strcmp(cmd[0].tokens, "exit"))
            {
                return (0);
            }
            commands(cmd);
        }
    }
    else
    {
        printf(".sushrc does not exist in $HOME\n");
    }


    while (1) //continue accepting input until user exits
    {
        char *ps1 = getenv("PS1"); //get PS1 variable if exists
        if (ps1 != NULL)
        {
            printf("%s ", ps1); //print PS1
        }
        else
        {
            printf("$ "); //else print $
        }
        line[0] = '\0';
        fgets(line, MAX_LIM, stdin); //takes string from stdin
        clear(cmd);

        if(line[0] == '\n') {
            continue;
        } else if(line[0] == '\0' || !strcmp(line, "exit\n")) {
            accnt();
            return 0;
        } else token(cmd, line);
        
        commands(cmd);

    }

    return 0;
}
