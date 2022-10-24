#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <signal.h>
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
void addNullChar(char *buf)
{
    int i = 1;
    int num;
    int pairsOfParantheses = 0;
    int pow;
    int maxPos = 0;
    while (i < strlen(buf))
    {
        num = 0;
        pow = 1;
        while (isdigit(buf[i]) != 0 && (buf[i - 1] == '(' || buf[i - 2] == '(' || buf[i - 3] == '(') && (buf[i + 1] == ')' || buf[i + 2] == ')' || buf[i + 3] == ')'))
        {
            num = num * pow + (buf[i] - '0');
            i++;
            pow *= 10;
        }
        // printf("\nNum:%d\n", num);
        i = i + num + 1;
        if (num > 0 && num < 999)
        {
            pairsOfParantheses++;
            maxPos += num;
        }
        if (num != 0 && buf[i + 1] != '(')
            break;

        // printf("\nbuf i:%c\n", buf[i + 1]);
        // printf("\n i:%d\n", i);
    }
    // printf("\nparanth:%d\n", pairsOfParantheses);
    // printf("\nmxNum:%d\n", maxPos + 5 * pairsOfParantheses);

    buf[maxPos + 5 * pairsOfParantheses] = '\0';
}
void startClientProcess()
{
    char buf[1024];
    int len;
    int fd;
    createFifo();
    write(1, "\n", 1);

    len = read(0, buf, 1024);
    if (len == -1)
    {
        perror("Err read:");
        exit(1);
    }
    buf[len] = '\0';

    if ((fd = open("fifoFile.txt", O_WRONLY)) == -1)
    {
        perror("Err open: ");
        exit(1);
    }

    if (write(fd, buf, strlen(buf)) == -1)
    {
        perror("Err write");
        exit(1);
    }
    close(fd);

    if ((fd = open("fifoFile.txt", O_RDONLY)) == -1)
    {
        perror("Err open: ");
        exit(1);
    }
    if (read(fd, buf, 1024) == -1)
    {
        perror("Err read:");
        exit(1);
    }
    close(fd);

    addNullChar(buf);

    write(1, "\n", 1);
    if (write(1, buf, strlen(buf)) == -1)
    {
        perror("Err write");
        exit(1);
    }

    if (strstr(buf, "Quit") != 0)
        exit(0);
}
int main(void)
{

    write(1, "--------------\n|CLIENT START|\n--------------", 46);
    while (1)
    {
        startClientProcess();
    }

    return 0;
}
