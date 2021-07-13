#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#define PORT 8080

void requestFile(int sock, char *filename)
{
    char *buffer = (char *)malloc(sizeof(char) * 8192);
    printf("Requesting %s\n", filename);
    send(sock, filename, strlen(filename), 0);

    int valread = read(sock, buffer, 8192);
    if (valread < 0)
    {
        perror("Read failed");
        free(buffer);
        exit(1);
    }
    buffer[valread] = '\0';

    if (strcmp(buffer, "ERROR") == 0)
    {
        printf("File requested doesnt exist\n"); 
        free(buffer);
        return;
    }

    int f = open(filename, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IXUSR); 
    while (1)
    {
        write(f, buffer, valread); 
        if (valread < 8192)
        {
            break;
        }
        valread = read(sock, buffer, 8192); 
        if (valread < 0)
        {
            free(buffer);
            close(f);
            perror("Filed to read the file");
            exit(1);
        }
        buffer[valread] = '\0';
    }
    free(buffer);
    close(f);
    printf("\nFile downloaded\n");
    return;
}

char **parse_semi(char *input)
{
    char **a = malloc(16 * sizeof(char *));
    int in_size = 1024, x = 0;
    const char delim[2] = " ";
    char *token;

    token = strtok(input, delim);
    while (token != NULL)
    {
        a[x] = token;
        x++;
        token = strtok(NULL, delim);
    }
    a[x] = token;
    return a;
}

int main(int argc, char const *argv[])
{
    struct sockaddr_in address;
    int sock = 0;
    struct sockaddr_in serv_addr;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) //socket creation error
    {
        printf("\n Socket creation error \n");
        return -1;
    }

    memset(&serv_addr, '0', sizeof(serv_addr)); // to make sure the struct is empty. Essentially sets sin_zero as 0
                                                // which is meant to be, and rest is defined below

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Converts an IP address in numbers-and-dots notation into either a
    // struct in_addr or a struct in6_addr depending on whether you specify AF_INET or AF_INET6.
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) // connect to the server address
    {
        printf("\nConnection Failed \n");
        return -1;
    }

    for (int i = 0;; i++)
    {
        printf("client>");
        int in_size = 1024, x = 0;
        char *input = (char *)malloc(1024);
        int c;

        if (!input)
        {
            perror("buffer");
            break;
        }

        while (1)
        {
            c = getchar();

            if (c == EOF || c == '\n')
            {
                input[x] = '\0';
                break;
            }
            else
                input[x] = c;
            x++;

            if (x >= in_size)
            {
                in_size += 1024;
                input = realloc(input, in_size);
                if (!input)
                {
                    perror("buffer");
                    return 0;
                }
            }
        }

        char **a = parse_semi(input);

        if (a[0] == NULL)
            break;
        if (strcmp(a[0], "exit") == 0)
        {
            send(sock, "LEAVE", 4, 0);
            break;
        }
        else if (strcmp(a[0], "get") == 0)
        {
            for (int j = 1;; j++)
            {
                if (a[j] == NULL)
                    break;
                requestFile(sock, a[j]);
            }
        }
        else
            printf("Invalid command.\n");
    }

    return 0;
}