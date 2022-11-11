#include"so_stdio.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include<stdio.h>
#include<string.h>
#include <sys/wait.h>
#define BUF_SIZE 4096
struct _so_file
{
    int fd;//file descriptor
    unsigned char buffer[BUF_SIZE];//4096 de bytes din fisier odata
    char* open;//modul de deschidere al fisierului
    int EOFile;//daca am ajuns la finalul fisierului
    int _error;//daca intalnesc vreo eroare
    int PosInFile;//cursor
    int PosInBuffer;//cat am ocupat din buffer;
    int bytes;
    char lastOp[200];//ultima operatie pentru a sti daca este nevoie sa facem so_fflush sau so_fseek
    pid_t child;//pid-ul procesului copil creat dupa so_popen
};
SO_FILE* so_fopen(const char* pathname, const char* mode)
{
    int flag = -1;
    if (!strcmp(mode, "r"))
    {
        flag = O_RDONLY;
    }
    else if (!strcmp(mode, "r+"))
    {
        flag = O_RDWR;
    }
    else if (!strcmp(mode, "w"))
    {
        flag = O_WRONLY | O_CREAT | O_TRUNC;
    }
    else if (!strcmp(mode, "w+"))
    {
        flag = O_RDWR | O_CREAT | O_TRUNC;
    }
    else if (!strcmp(mode, "a"))
    {
        flag = O_APPEND | O_WRONLY | O_CREAT;
    }
    else if (!strcmp(mode, "a+"))
    {
        flag = O_APPEND | O_RDWR | O_CREAT;
    }
    if (flag == -1)
    {
        printf("Invalid flag for open file\n*\t*\t*\n");
        return NULL;
    }
    int fd = open(pathname, flag, 0644);
    if (fd == -1)
    {
        printf("Open operation didn't work\n*\t*\t*\n");
        return NULL;
    }
    SO_FILE* fis = (SO_FILE*)malloc(sizeof(SO_FILE));
    if (fis == NULL)
    {
        printf("Unable to allocate memory");
        return NULL;
    }
    fis->open = (char*)malloc(strlen(mode) + 1);
    if (fis->open == NULL)
    {
        printf("Unable to allocate memory");
        fis->_error = 1;
        return NULL;
    }
    fis->fd = fd;
    strcpy(fis->open, mode);
    strcpy(fis->buffer, "");
    strcpy(fis->lastOp, "");
    fis->EOFile = 0;
    fis->PosInBuffer = 0;
    fis->PosInFile = 0;
    fis->_error = 0;
    fis->bytes = 0;
    return fis;
}

int so_fclose(SO_FILE* stream)
{

    if (stream != NULL) {
        if (!strcmp(stream->lastOp, "write")) {
            int ret = so_fflush(stream);
            if (ret == -1)
                return SO_EOF;
        }
        close(stream->fd);
        if (stream->open != NULL)
            free(stream->open);
        else
            return SO_EOF;
        free(stream);
        stream = NULL;
        return 0;
    }
    else
    {
        return SO_EOF;
    }
}

int so_fgetc(SO_FILE* stream)
{
    if (stream->EOFile == 1)
        return SO_EOF;
    if (stream == NULL || !strcmp(stream->open, "w") || !strcmp(stream->open, "a")) {
        printf("Operation is not valid\n*\t*\t*\n");
        stream->_error = 1;
        return SO_EOF;
    }

    if (!strcmp(stream->lastOp, "write")) {

        so_fflush(stream);
        stream->bytes = read(stream->fd, stream->buffer, BUF_SIZE);
        if (stream->bytes == -1)
        {
            stream->_error = 1;
            return SO_EOF;
        }
        else if (stream->bytes == 0)
        {
            stream->EOFile = 1;
            return SO_EOF;
        }
    }
    strcpy(stream->lastOp, "read");
    if (stream->PosInBuffer == 0 && strcmp(stream->buffer, "") == 0)
    {

        stream->bytes = read(stream->fd, stream->buffer, BUF_SIZE);
        if (stream->bytes == 0) {
            stream->EOFile = 1;
            return SO_EOF;
        }
        stream->PosInFile++;
        return stream->buffer[stream->PosInBuffer++];
    }
    else if (stream->PosInFile != 0 && stream->EOFile == 0)
    {
        if (stream->PosInBuffer == BUF_SIZE)
        {
            stream->PosInBuffer = 0;
            stream->bytes = read(stream->fd, stream->buffer, BUF_SIZE);
            if (stream->bytes == 0) {
                stream->EOFile = 1;
                return SO_EOF;
            }
        }
        else if (stream->PosInBuffer == stream->bytes)
        {
            stream->EOFile = 1;
            return -1;
        }
        stream->PosInFile++;
        return stream->buffer[stream->PosInBuffer++];

    }
    stream->_error = 1;
    return SO_EOF;
}

int so_fputc(int c, SO_FILE* stream)
{
    int bw = 0;
    if (stream == NULL || !strcmp(stream->open, "r")) {
        printf("Operation is not valid\n*\t*\t*\n");
        stream->_error = 1;
        return SO_EOF;
    }

    if (!strcmp(stream->lastOp, "read")) {
        strcpy(stream->buffer, "");
        stream->PosInBuffer = 0;
        so_fseek(stream, stream->PosInFile, SEEK_SET);
    }
    strcpy(stream->lastOp, "write");

    if (stream->PosInBuffer == BUF_SIZE)
    {
        bw = so_fflush(stream);
        if (bw == -1)
        {
            stream->_error = 1;
            return SO_EOF;
        }
        strcpy(stream->buffer, "");
        stream->PosInBuffer = 0;
    }
    stream->buffer[stream->PosInBuffer++] = (unsigned char)c;
    stream->PosInFile++;
    return c;
}

size_t so_fread(void* ptr, size_t size, size_t nmemb, SO_FILE* stream)
{

    size_t total = size * nmemb;
    size_t i = 0;
    int car;
    unsigned char* aux = (unsigned char*)ptr;
    while (i < total)
    {
        car = so_fgetc(stream);
        if (car != -1)
            aux[i] = (unsigned char)car;
        else {
            stream->_error = 1;
            return 0;
        }
        if (stream->EOFile == 1||stream->_error == 1)
        {
            return 0;
        }
        i++;
    }
    size_t nrelements = i / size;
    return nrelements;

}

size_t so_fwrite(const void* ptr, size_t size, size_t nmemb, SO_FILE* stream)
{

    int i = 0;
    unsigned char* aux = (unsigned char*)ptr;
    int total = size * nmemb;
    int put = 0;
    while (i < total)
    {
        put = so_fputc((int)aux[i], stream);
        if (put == -1)
        {
            stream->_error = 1;
            break;
        }
        if(stream->_error==1)
        {
            break;
        }
        i++;
    }
    if (i / size != nmemb) {
        stream->_error = 1;
        return i / size;
    }
    return nmemb;
}

int so_fseek(SO_FILE* stream, long offset, int whence)
{
    if (!strcmp(stream->lastOp, "read"))
    {
        strcpy(stream->buffer, "");
        stream->PosInBuffer = 0;
    }
    else if (!strcmp(stream->lastOp, "write")) {
        int ret = so_fflush(stream);
        if (ret == SO_EOF)
        {
            return SO_EOF;
        }
    }
    int pos = lseek(so_fileno(stream), offset, whence);
    if (pos < 0)
        return SO_EOF;
    stream->PosInFile = pos;
    return 0;
}

long so_ftell(SO_FILE* stream)
{
    if (stream->_error != 0)
    {
        return -1;
    }
    return stream->PosInFile;
}

int so_fflush(SO_FILE* stream)
{
    if (!strcmp(stream->lastOp, "write"))
    {
        int bytes_written = 0;
        while (bytes_written < stream->PosInBuffer) {
            int nr = write(stream->fd, stream->buffer + bytes_written, stream->PosInBuffer - bytes_written);
            if (nr == 0)
                break;
            bytes_written += nr;
        }
        if (bytes_written == 0)
        {
            return SO_EOF;
        }
        strcpy(stream->buffer, "");
        strcpy(stream->lastOp, "");
        stream->PosInBuffer = 0;
        return 0;
    }
    else {
        return SO_EOF;
    }
}

int so_fileno(SO_FILE* stream)
{
    return stream->fd;
}

int so_feof(SO_FILE* stream)
{
    return stream->EOFile;
}

int so_ferror(SO_FILE* stream)
{
    //if(stream!=NULL)
        // return stream->_error;
    return 1;
}
SO_FILE* so_popen(const char* command, const char* type)
{
    SO_FILE* f = (SO_FILE*)malloc(sizeof(SO_FILE));
    if (f == NULL)
    {
        return NULL;
    }
    f->open = (char*)malloc(strlen(type) + 1);
    if (f->open == NULL)
    {
        free(f);
        return NULL;
    }
    strcpy(f->open, type);
    strcpy(f->buffer, "");
    strcpy(f->lastOp, "");
    f->PosInFile = 0;
    f->PosInBuffer = 0;
    f->_error = 0;
    f->EOFile = 0;
    f->PosInBuffer = 0;
    f->PosInFile = 0;
    f->bytes = 0;
    f->fd = -1;
    int fd[2];
    int ret_val = pipe(fd);
    if (ret_val < 0)
    {
        free(f);
        return NULL;
    }
    if (strcmp(type, "r") == 0)
        f->fd = fd[0];//retinem fd-ul corespunzator operatiei
    else if (strcmp(type, "w") == 0)
        f->fd = fd[1];//retinem fd-ul corespunzator operatiei

    f->child = fork();
    if (f->child < 0)
    {
        close(fd[0]);
        close(fd[1]);
        free(f->open);
        free(f);
        return NULL;
    }
    else if (f->child == 0)
    {
        if (strcmp(type, "r") == 0)
        {
            close(fd[0]);//daca in procesul parinte citesc,in procesul copil scriu, deci pot inchide capatul de citire din copil
            dup2(fd[1], STDOUT_FILENO);//stdout va fi inlocuit de file descriptorul corespunzator pentru capatul pipeului de write
            close(fd[1]);//putem inchide intrarea din fdt corespunzatoare capatului de scriere pentru ca am copiat in stdout
        }
        else if (strcmp(type, "w") == 0)
        {
            close(fd[1]);//daca in procesul parinte scriu, in procesul copil citesc, deci pot inchide capatul de scriere din copil
            dup2(fd[0], STDIN_FILENO);//stdin va fi inlocuit de file descriptorul corespunzator pentru capatul pipeului de scriere
            close(fd[0]);//putem inchide intrarea din fdt corespunzatoare capatului de citire pentru ca am copiat in stdin
        }
        execl("/bin/bash", "sh", "-c", command, NULL);//executam comanda in bash
        exit(1);

    }
    else {
        //variabila pentru a retine file descriptorul corespunzator capatului de pipe
        if (strcmp(type, "r") == 0)
        {
            close(fd[1]);//daca in procesul parinte citim, putem inchide capatul pentru scriere

        }
        else if (strcmp(type, "w") == 0)
        {
            close(fd[0]);//daca in procesul parinte scriem, putem inchide capatul de citire
        }
        return f;
    }
    if (f != NULL)
        free(f);
    return NULL;

}
int so_pclose(SO_FILE* stream)
{
   
    int status;
    int ret = waitpid(stream->child, &status, 0);
    if (ret < 0)
    {
        so_fclose(stream);
        return SO_EOF;
    }
    ret = so_fclose(stream);
    if (ret < 0)
        return SO_EOF;
    return status;
}




