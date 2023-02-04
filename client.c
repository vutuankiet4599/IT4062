#include <dirent.h>
#include <string.h>
#include <malloc.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <libgen.h>
#include <pthread.h>

// debug
char debug[1024] = "linh debug\n";

// socket udp bat ip
#define INVALID_SOCKET -1
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
char listClient[1000][20];
int countClient = 0;
int checkClientExist = 0;

// tcp
int cfd;

// flag de tat tat ca cac luon
void sighandler(int signum)
{
    int stat = 0;
    while (waitpid(-1, &stat, WNOHANG) > 0)
        ;
}

/**
 * lang nghe cong khac, luu thong tin vao file txt cac client tham gia vao mang
 */
void listenFromPeer()
{
    int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET)
    {
        perror("socket");
    }
    SOCKADDR_IN saddr, ackaddr, caddr;
    int clen = sizeof(caddr);
    ackaddr.sin_family = AF_INET;
    ackaddr.sin_port = htons(5001); // quy dinh nhan goi tin broad cast o 5001
    ackaddr.sin_addr.s_addr = 0;
    bind(s, (SOCKADDR *)&ackaddr, sizeof(ackaddr));

    // saddr.sin_family = AF_INET;
    // saddr.sin_port = htons(5000);
    // saddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    while (1)
    {
        // sleep(2);
        // char buffer[1024] = {0};
        char ack[1024] = {0};
        int r = recvfrom(s, ack, sizeof(ack), 0, (SOCKADDR *)&caddr, &clen);
        if (r > 0)
        {
            // Xu li ip trung thi khong them vao trong file
            for (int i = 0; i < countClient; i++)
            {
                if (strcmp(listClient[i], inet_ntoa(caddr.sin_addr)) == 0)
                    checkClientExist = 1;
            }

            if (!checkClientExist)
            {
                // ghi vao array string de lan sau check
                strcpy(listClient[countClient], inet_ntoa(caddr.sin_addr));
                countClient++;
                // ghi vao file
                FILE *f = fopen("clients.txt", "a+");
                fprintf(f, "%d. %s %s\n", countClient, ack, inet_ntoa(caddr.sin_addr));
                fclose(f);
                // reset cho lan tiep theo
                checkClientExist = 0;
            }
        }
    }
    close(s);
}

// tra ve ip cua client
char *readSpecificClient(int lineNumber)
{
    FILE *file = fopen("clients.txt", "r");
    int count = 0;
    if (file != NULL)
    {
        char *returnLine = (char *)calloc(256, sizeof(char));
        char line[256];
        while (fgets(line, sizeof(line), file) != NULL) /* read a line */
        {
            if (count == lineNumber)
            {
                return strcpy(returnLine, line);
            }
            else
            {
                count++;
            }
        }
    }
    else
    {
        printf("Passing error file");
    }
    fclose(file);
}

void connectToPeerServer()
{
    int clientNumber;
    printf("Please enter client number to connect:");
    scanf("%d", &clientNumber);
    char *line = readSpecificClient(clientNumber - 1);
    char s1[50];
    char s2[50];
    char s3[50];
    char s4[50];
    sscanf(line, "%s%s%s%s", s1, s2, s3, s4);
    printf("ip: %s\n", s3);
    printf("%d", inet_addr(s3));
    // create connect tcp to server
    cfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // connect server
    SOCKADDR_IN saddr, caddr;
    // int clen = sizeof(caddr);
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(6001);
    saddr.sin_addr.s_addr = inet_addr(s3);
    connect(cfd, (SOCKADDR *)&saddr, sizeof(saddr));
    send(cfd, "download1", 9, 0);
    send(cfd, "download2", 9, 0);
}

void dowload()
{
    send(cfd, "download", 8, 0);
}
void upload()
{
    send(cfd, "upload", 6, 0);
}

void back()
{
    close(cfd);
}

void selectService()
{
    int key;
    do
    {
        printf("**service**\n");
        printf("1. say dowload\n");
        printf("2. say upload\n");
        printf("3. back\n");
        printf("Enter your choice:");
        scanf("%d", &key);
        switch (key)
        {
        case 1:
            dowload();
            break;
        case 2:
            upload();
            break;
        case 3:
            back();
            break;
        default:
            break;
        }
    } while (key > 0 && key < 3);

    // close(cfd);
}

void choiceHandle(int chooseNumber)
{
    FILE *f;
    switch (chooseNumber)
    {
    case 1:
        printf("You are in case 1\n");
        connectToPeerServer();
        selectService();
        break;
    case 2:
        f = fopen("clients.txt", "r");
        printf("List of client:\n");
        while (!feof(f))
        {
            char buffer[1024] = {0};
            fgets(buffer, sizeof(buffer), f);
            printf("%s", buffer);
        }
        fclose(f);
        printf("\n\n");
        break;
    case 3:
        printf("Exit program!\n");
        exit(0);
    default:
        printf("Please enter correct number");
        break;
    }
    return;
}

void displayMenu()
{
    sleep(3);
    int chooseNumber;
    do
    {
        printf("------------------------------\n");
        printf("1.Choose user\n");
        printf("2.Reload list client\n");
        printf("3.Exit\n");
        printf("------------------------------\n");
        printf("Please enter your choice:\n");
        scanf("%d", &chooseNumber);
        printf("You choosed %d!!\n", chooseNumber);
        choiceHandle(chooseNumber);
    } while (chooseNumber > 0 && chooseNumber < 4);
}

int main()
{

    signal(SIGCHLD, sighandler);
    // Ket thuc chuong trinh
    printf("Welcome to file sharing app!!\n");

    // reset file
    FILE *f = fopen("clients.txt", "w");
    fclose(f);
    // 2 luong chinh nhu sau
    // 1. Bat udp cua tat ca cac thg tham gia trong mang vao file client.txt
    // 2. hien thi thong tin udp sau 3 giay
    if (fork() == 0)
    {
        listenFromPeer();
    }
    else
    {
        if (fork() == 0)
        {
            displayMenu();
        }
        else
        {
            sleep(1000);
        }
    }

    // Hien bang dieu khien UPLOAD
    // displayMenu();

    // Tao kiet noi tcp de thang kiet lorcak gui du lieu ve

    return 0;
}