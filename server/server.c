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

int SendData(int fd, char *data, int len)
{
    int sent = 0;
    int tmp = 0;
    do
    {
        tmp = send(fd, data + sent, len - sent, 0);
        sent += tmp;
    } while (tmp >= 0 && sent < len);
    return sent;
}

int RecvData(int fd, char *data, int maxlen)
{
    int received = 0;
    int blocksize = 1024;
    int tmp = 0;
    do
    {
        tmp = recv(fd, data + received, blocksize, 0);
        received += tmp;
    } while (tmp >= 0 && received < maxlen && tmp == blocksize);
    return received;
}

void AppendString(char **dest, const char *src)
{
    char *tmp_dest = *dest;
    int old_length = tmp_dest == NULL ? 0 : strlen(tmp_dest);
    *dest = (char *)realloc(*dest, old_length + strlen(src) + 1);
    tmp_dest = *dest;
    memset(tmp_dest + old_length, 0, strlen(src) + 1);
    sprintf(tmp_dest + old_length, "%s", src);
}

void BoardCasProcess()
{
    int bfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    struct sockaddr_in saddr;
    saddr.sin_addr.s_addr = inet_addr("255.255.255.255");
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(5001);
    int on = 1;
    setsockopt(bfd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));

    char hostBuffer[256];
    int host = gethostname(hostBuffer, sizeof(hostBuffer));
    struct hostent *hostEntry = gethostbyname(hostBuffer);
    char *IPAddress = inet_ntoa(*((struct in_addr *)hostEntry->h_addr_list[0]));
    char *mess = NULL;
    AppendString(&mess, "IPBroadcast ");
    AppendString(&mess, IPAddress);
    AppendString(&mess, " ");
    AppendString(&mess, hostBuffer);
    AppendString(&mess, " 6001");

    while (1 == 1)
    {
        int sent = sendto(bfd, mess, strlen(mess), 0, (struct sockaddr *)&saddr, sizeof(saddr));
        sleep(5);
    }

    free(mess);
    mess = NULL;
    close(bfd);
}

int alphasort(const struct dirent **a, const struct dirent **b)
{
    if ((*a)->d_type == DT_DIR && (*b)->d_type == DT_REG)
    {
        return -1;
    }
    if ((*a)->d_type == (*b)->d_type)
    {
        return 0;
    }
    return 1;
}

void SendListFileToClient(int cfd)
{
    char *path = NULL;
    char _path[1024] = {0};
    struct dirent **listFile = NULL;
    getcwd(_path, 1024);
    AppendString(&path, _path);
    AppendString(&path, "/FileStorages/");
    int n = scandir(path, &listFile, NULL, alphasort);
    char *mess = NULL;
    if (n <= 2)
    {
        AppendString(&mess, "Empty");
    }
    else
    {
        for (int i = 0; i < n; i++)
        {
            if (listFile[i]->d_type == DT_REG)
            {
                AppendString(&mess, listFile[i]->d_name);
                AppendString(&mess, "\n");
            }
        }
    }
    SendData(cfd, mess, strlen(mess));
    free(mess);
    mess = NULL;
    SendData(cfd, "END", 3);
}

int AnalyisMessage(char *_mess)
{
    char mess[1024] = {0};
    for (int i = 0; i < strlen(_mess) && _mess[i] != ' '; i++)
    {
        mess[i] = toupper(_mess[i]);
    }

    if (strncmp(mess, "UPLOAD", 6) == 0)
        return 1;
    if (strncmp(mess, "DOWNLOAD", 8) == 0)
        return 2;
    if (strncmp(mess, "LIST", 4) == 0)
        return 3;
    if (strncmp(mess, "DONE", 4) == 0)
        return 4;

    return 0;
}

void HandleUpload(int cfd, char *filePathClient)
{
    char *filePath = NULL;
    char *fileName = basename(filePathClient);
    char path[1024] = {0};
    getcwd(path, 1024);
    AppendString(&filePath, path);
    AppendString(&filePath, "/FileStorages/");
    AppendString(&filePath, fileName);
    char *err = NULL;

    if (access(filePath, F_OK) == 0)
    {
        err = "File is existed!";
        SendData(cfd, err, strlen(err));
    }
    else
    {
        FILE *f = fopen(filePath, "ab");
        int received = 0;
        char buffer[1024] = {0};
        do
        {
            memset(buffer, 0, 1024);
            received = RecvData(cfd, buffer, 1024);
            FILE *t = fopen("helllo.txt", "w+");
            fprintf(t, "Buffer %s\n", buffer);
            fclose(t);
            if (AnalyisMessage(buffer) == 4)
                break;
            fwrite(buffer, strlen(buffer), 1, f);
        } while (received > 0);
        fclose(f);
    }
}

void HandleDownload(int cfd, char *fileName)
{
    char *filePath = NULL;
    char path[1024] = {0};
    getcwd(path, 1024);
    AppendString(&filePath, path);
    AppendString(&filePath, "/FileStorages/");
    AppendString(&filePath, fileName);
    FILE *f = fopen(filePath, "rb");
    if (f == NULL)
    {
        char *err = "File not found!";
        SendData(cfd, err, strlen(err));
        fclose(f);
        return;
    }

    fseek(f, 0, SEEK_END);
    int fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *data = (char *)calloc(fsize, 1);
    fread(data, 1, fsize, f);
    fclose(f);
    SendData(cfd, data, fsize);
    free(data);
    data = NULL;
    SendData(cfd, "END", 3);
}

void HandleProcess(int cfd)
{
    while (1 == 1)
    {
        char mess[1024] = {0};
        int received = RecvData(cfd, mess, 1024);
        if (received > 0)
        {
            while (mess[strlen(mess) - 1] == '\n' || mess[strlen(mess) - 1] == '\r')
            {
                mess[strlen(mess) - 1] = 0;
            }
            int cmd = AnalyisMessage(mess);
            char *fileName = NULL;
            fileName = strtok(mess, " ");
            fileName = strtok(NULL, " ");
            switch (cmd)
            {
            case 1:
                if (fileName == NULL)
                {
                    char *err = "File name required!";
                    SendData(cfd, err, strlen(err));
                }
                else
                {
                    HandleUpload(cfd, fileName);
                }
                break;
            case 2:
                if (fileName == NULL)
                {
                    char *err = "File name required!";
                    SendData(cfd, err, strlen(err));
                }
                else
                {
                    HandleDownload(cfd, fileName);
                }
                break;
            case 3:
                SendListFileToClient(cfd);
                break;

            default:
                SendData(cfd, "WRONG COMMAND", 13);
                break;
            }
        }
    }
    close(cfd);
}

void ServerProcess()
{
    int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in saddr, caddr;
    int clen = sizeof(caddr);
    saddr.sin_addr.s_addr = 0;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(6001);
    int err = bind(fd, (struct sockaddr *)&saddr, sizeof(saddr));
    if (err == -1)
    {
        printf("1: Error!\n");
        return;
    }
    err = listen(fd, 10);
    if (err == -1)
    {
        printf("2: Error!\n");
    }

    while (1 == 1)
    {
        int cfd = accept(fd, (struct sockaddr *)&caddr, &clen);
        if (fork() == 0)
        {
            SendListFileToClient(cfd);
            HandleProcess(cfd);
        }
    }
    close(fd);
}

int main()
{
    if (fork() == 0)
    {
        BoardCasProcess();
    }
    else
    {
        if (fork() == 0)
        {
            ServerProcess();
        }
        else
        {
            sleep(1000);
        }
    }
    return 0;
}
