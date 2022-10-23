#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <utmp.h>

#define READ 0
#define WRITE 1

// #define USERVALID 1
// #define USERINVALID 2
// #define INVALIDCOMMAND 3
// #define MUSTBELOGGED 4
// #define QUIT 5
// #define EMPTYINPUT 6
// #define ALREADYLOGGEDIN 7

//#define PARENT 1
//#define CHILD 2

#define TRUE 1
#define FALSE 0

int loggedIn = 0;
int quit = 0;

void createFifo()
{
    if (access("fifoFile.txt", F_OK) == -1)
    {
        write(1, "#FIFO CREATED#\n", 15);
        if (mkfifo("fifoFile.txt", 0666) == -1)
        {
            perror("Error ");
            exit(1);
        }
    }
}
void intToString(int number, char *newNumber)
{
    int i = 0, length = 0, n;
    n = number;

    while (n != 0)
    {
        length++;
        n /= 10;
    }
    while (i < length)
    {
        newNumber[length - i - 1] = number % 10 + '0';
        number = number / 10;
        i++;
    }
    newNumber[length] = '\0';
}

void prefixOutput(char *output, char *prefixedOutput)
{
    char length[256] = "";

    output[strlen(output)] = '\0';
    intToString(strlen(output), length);
    strcpy(prefixedOutput, "(");
    strcat(prefixedOutput, length);
    strcat(prefixedOutput, ") ");
    strcat(prefixedOutput, output);
    // write(1,prefixedOutput,strlen(prefixedOutput));
    prefixedOutput[strlen(prefixedOutput)] = '\0';
}
void prepareOutput(char *message, char *prefixedMessage, char *messageToPrint)
{
    strcpy(prefixedMessage, "");
    strcpy(message, messageToPrint);
    prefixOutput(message, prefixedMessage);
}

void printMessageToClient(char *message, int fd)
{
    char prefixedMessage[1024];

    strcpy(prefixedMessage, message);

    if (strcmp(message, "USERVALID") == 0)
    {
        prepareOutput(message, prefixedMessage, "User successfully connected!\n");

        loggedIn = 1;
    }
    else if (strcmp(message, "USERINVALID") == 0)
    {
        prepareOutput(message, prefixedMessage, "User is invalid or has no access rights.\n");
    }

    else if (strcmp(message, "INVALIDCOMMAND") == 0)
    {
        prepareOutput(message, prefixedMessage, "Command does not exist or uses wrong format.\n");
    }

    else if (strcmp(message, "MUSTBELOGGED") == 0)
    {
        prepareOutput(message, prefixedMessage, "You must be logged to use that command!\n");
    }

    else if (strcmp(message, "QUIT") == 0)
    {
        prepareOutput(message, prefixedMessage, "Quitting.\n");

        quit = 1;
    }
    else if (strcmp(message, "LOGOUT") == 0)
    {
        prepareOutput(message, prefixedMessage, "Logging out.\n");

        loggedIn = 0;
    }
    else if (strcmp(message, "EMPTYINPUT") == 0)
    {
        prepareOutput(message, prefixedMessage, "Please enter a command.\n");
    }

    else if (strcmp(message, "ALREADYLOGGEDIN") == 0)
    {
        prepareOutput(message, prefixedMessage, "ALREADY LOGGED IN!\n");
    }
    else if (strcmp(message, "INVALIDPID") == 0)
    {
        prepareOutput(message, prefixedMessage, "Pid is invalid!\n");
    }

    if(write(fd, prefixedMessage, strlen(prefixedMessage))==-1)
    {
        perror("Err write:");
        exit(1);
    }
    
}

void parentProcess(int readFd, int writeFd)
{
    char buf[1024];
    int fd;
    int readLength;

    createFifo();

    if((fd = open("fifoFile.txt", O_RDONLY))==-1){
        perror("Err open:");
        exit(1);
    }

    readLength = read(fd, buf, 1024);
    if(readLength==-1)
    {
        perror("Err read:");
        exit(1);
    }
    buf[readLength - 1] = '\0';
    close(fd);
    if(write(writeFd, buf, readLength)==-1)
    {
        perror("Err write:");
        exit(1);
    }

    readLength = read(readFd, buf, 1024);
    if(readLength==-1)
    {
        perror("Err read:");
        exit(1);
    }
    buf[readLength] = '\0';

    printf("\n<--- MSG READ FROM CHILD --->\n");
    write(1, buf, strlen(buf));
    printf("\n<--- MSG READ FROM CHILD --->\n");

    if((fd = open("fifoFile.txt", O_WRONLY))==-1)
    {
        perror("Err open:");
        exit(1);
    }

    printMessageToClient(buf, fd);
    close(fd);
    int status;
    wait(&status);
  
}

int verifyIfUserExists(char *user)
{
    FILE *fp;
    char buf[256];
    fp = fopen("users.txt", "r");
    if (fp == NULL)
    {
        perror("users.txt missing ");
        exit(0);
    }

    while (fgets(buf, 256, fp))
    {
        buf[strlen(buf) - 1] = '\0';

        if (strcmp(buf, user) == 0)
        {
            return TRUE;
        }
    }
    fclose(fp);

    return FALSE;
}
int formatString(char *string, char *newString)
{
    char *p;
    int colonUsed = 0;
    p = strtok(string, " ");
    int substringAfterColonCount = 0;
    while (p != NULL)
    {
        if (colonUsed == 1)
            substringAfterColonCount++;
        if (strcmp(p, ":") == 0 || p[strlen(p) - 1] == ':')
            colonUsed = 1;
        else if (strstr(p, ":") != 0 && p[strlen(p) - 1] != ':')
        {
            colonUsed = 1;
            substringAfterColonCount = 1;
        }
        strcat(newString, p);
        p = strtok(NULL, " ");
    }
    return substringAfterColonCount;
}
char *validateLoginCommand(char *user, int substringAfterColonCount)
{
    if (substringAfterColonCount > 1 || substringAfterColonCount==0)
        return "USERINVALID";
    strcpy(user, user + 6);
    if (verifyIfUserExists(user) == TRUE)
        return "USERVALID";
    return "USERINVALID";
}
void getLoggedUsers(int writeFd)
{
    struct utmp user;
    int fd;
    char response[1024] = "";
    char currentUser[1024] = "";
    char seconds[1024];
    char temp[1024] = "";
    char miliseconds[1024];

    if ((fd = open(UTMP_FILE, O_RDONLY)) == -1)
    {
        perror(UTMP_FILE);
        exit(1);
    }

    while (read(fd, &user, sizeof(user)) == sizeof(user))
    {
        strcpy(currentUser, "");
        strcpy(seconds, "");
        strcpy(miliseconds, "");
        strcpy(temp, "");

        intToString(user.ut_tv.tv_sec, seconds);
        intToString(user.ut_tv.tv_usec, miliseconds);
        // printf("%s %s %s %s\n", user.ut_user, user.ut_host, seconds, miliseconds);

        strcat(currentUser, user.ut_user);
        strcat(currentUser, "-----");
        strcat(currentUser, user.ut_host);
        strcat(currentUser, "-----");
        strcat(currentUser, seconds);
        strcat(currentUser, "-----");
        strcat(currentUser, miliseconds);
        strcat(currentUser, "\n");

        prefixOutput(currentUser, temp);
        strcat(response, temp);
    }

    response[strlen(response)] = '\0';
    write(writeFd, response, strlen(response));
    // printf("\nsize:%d\n", strlen(response));
    close(fd);
}
void getProcInfo(int writeFd, char *pid, int substringAfterColonCount)
{
    char path[1024] = "/proc/";
    FILE *fp;
    char buf[1024];
    char response[1024] = "";

    if (substringAfterColonCount > 1 || substringAfterColonCount==0)
        write(writeFd, "INVALIDPID", 10);
    else
    {
        strcpy(pid, pid + 14);
        strcat(path, pid);
        strcat(path, "/status");
        printf("\n%s\n", path);
        if ((fp = fopen(path, "r")) == NULL)
            write(writeFd, "INVALIDPID", 10);
        while (fgets(buf, 256, fp))
        {
            char tempString[1024] = "Process ";
            buf[strlen(buf) - 1] = '\0';
            if (strstr(buf, "Name:") || strstr(buf, "State:") || strstr(buf, "PPid") || strstr(buf, "Uid") || strstr(buf, "VmSize:"))
            {
                strcat(tempString, buf);
                strcat(tempString, "\n");

                strcpy(buf, tempString);
                prefixOutput(buf, tempString);

                strcat(response, tempString);
            }
        }
        write(writeFd, response, strlen(response));
        fclose(fp);
    }
 
}
char *checkInputInfo(char *buf, int writeFd)
{
    char newString[1024] = "";
    int substringAfterColonCount = 0;
    if (strcmp(buf, "") == 0)
        return "EMPTYINPUT";

    substringAfterColonCount = formatString(buf, newString);

    if (strstr(newString, "login:") && loggedIn == 0)
    {
        return validateLoginCommand(newString, substringAfterColonCount);
    }
    else if (strstr(newString, "login:") && loggedIn == 1)
    {
        return "ALREADYLOGGEDIN";
    }
    else if (strcmp(newString, "quit") == 0)
    {
        return "QUIT";
    }
    else if ((strcmp(newString, "get-logged-users") == 0 || strstr(newString, "get-proc-info:") != 0 || strcmp(newString, "logout") == 0) && loggedIn == 0)
        return "MUSTBELOGGED";
    else if (loggedIn == 1 && strcmp(newString, "logout") == 0)
        return "LOGOUT";
    else if (loggedIn == 1 && strcmp(newString, "get-logged-users") == 0)
    {
        getLoggedUsers(writeFd);
        return "SKIP";
    }
    else if (loggedIn == 1 && strstr(newString, "get-proc-info:") != 0)
    {
        getProcInfo(writeFd, newString, substringAfterColonCount);
        return "SKIP";
    }
    return "INVALIDCOMMAND";
}
void childProcess(int readFd, int writeFd)
{

    char buf[1024];
    int readLength = read(readFd, buf, 1024); 
    if(readLength==-1)
    {
        perror("Err read:");
        exit(1);
    }

    buf[readLength] = '\0';
    printf("\n<--- READ FROM PARENT --->\n");
    write(1, buf, readLength);
    printf("\n<--- READ FROM PARENT --->\n");

    strcpy(buf, checkInputInfo(buf, writeFd)); // <---

    if (strcmp(buf, "SKIP") != 0)
        write(writeFd, buf, strlen(buf));

    close(writeFd);
    close(readFd);
    exit(0);
}

void startServerProcess()
{
    int parentToChild[2];
    int childToParent[2];

    pipe(parentToChild);
    pipe(childToParent);

    pid_t pid = fork();

    if (pid < 0)
        exit(1);
    else if (pid > 0) // parent process
    {

        close(parentToChild[READ]);
        close(childToParent[WRITE]);

        parentProcess(childToParent[READ], parentToChild[WRITE]);

        close(childToParent[READ]);
        close(parentToChild[WRITE]);
    }
    else if (pid == 0) // child process
    {

        close(parentToChild[WRITE]);
        close(childToParent[READ]);

        childProcess(parentToChild[READ], childToParent[WRITE]);
    }
}

int main(void)
{

    write(1, "--------------\n|SERVER START|\n--------------\n", 47);
    while (quit == 0)
    {
        startServerProcess();
    }

    return 0;
}
