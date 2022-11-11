#define _CRT_SECURE_NO_WARNINGS
#include<stdio.h>
#include"so_stdio.h"
#include<Windows.h>
#include <sys/types.h>
#include <sys/stat.h>
#define BUF_SIZE 4096
struct _so_file
{
    HANDLE fd;//file descriptor
    char buffer[BUF_SIZE];//4096 de bytes din fisier odata
    char* open;//modul de deschidere al fisierului
    int EOFile;//daca am ajuns la finalul fisierului
    int error;//daca intalnesc vreo eroare
    DWORD PosInFile;//cursor
    DWORD PosInBuffer;//cat am ocupat din buffer;
    char lastOp[200];//ultima operatie pentru a putea sti cand sa facem so_fflush
    PROCESS_INFORMATION child;//informatii despre procesul creat dupa popen
};

SO_FILE* so_fopen(const char* pathname, const char* mode)
{
    DWORD flag;
    DWORD action;
    HANDLE fd;
    if (!strcmp(mode, "r"))
    {
        flag = GENERIC_READ;
    }
    else if (!strcmp(mode, "w"))
    {
        flag = GENERIC_WRITE;
    }
    else if (!strcmp(mode, "r+")||!strcmp(mode,"w+")||!strcmp(mode,"a")||!strcmp(mode,"a+"))
    {
        flag = GENERIC_WRITE | GENERIC_READ;
    }
    else
    {
        flag = -1;
    }
    if (flag == -1)
    {
        printf("Invalid flag for open file\n*\t*\t*\n");
        return NULL;
    }
    if (!strcmp(mode, "r") || !strcmp(mode, "r+"))
        action = OPEN_EXISTING;
    else if (!strcmp(mode, "w") || !strcmp(mode, "w+"))
        action = CREATE_ALWAYS;
    else if (!strcmp(mode, "a") || !strcmp(mode, "a+"))
        action = OPEN_ALWAYS;
    

    fd = CreateFileA((char*)pathname, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, action, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fd == INVALID_HANDLE_VALUE)
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
        fis->error = 1;
        return NULL;
    }
    fis->fd = fd;
    strcpy(fis->open, mode);
    strcpy(fis->buffer, "");
    strcpy(fis->lastOp, "");
    fis->EOFile = 0;
    fis->PosInBuffer = 0;
    fis->PosInFile = 0;
    fis->error = 0;
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
        CloseHandle(stream->fd);
        if (stream->open != NULL)
            free(stream->open);
        else
            return SO_EOF;
        if (stream->error == 1)
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
    DWORD bytes=0;
    int br = 1;
    if (stream->EOFile == 1)
        return SO_EOF;
    if (stream == NULL || !strcmp(stream->open, "w") || !strcmp(stream->open, "a")) {
        printf("Operation is not valid\n*\t*\t*\n");
        stream->error = 1;
        return SO_EOF;
    }

    if (!strcmp(stream->lastOp, "write")) {

        so_fflush(stream);
        br = ReadFile(stream->fd, stream->buffer, BUF_SIZE, &bytes, NULL);
        if (br == 0) {
            stream->error = 1;
            return SO_EOF;
        }
        if (bytes == 0) {
            stream->EOFile = 1;
            return SO_EOF;
        }

    }
    
    strcpy(stream->lastOp, "read");
    if (stream->PosInBuffer == 0 && strcmp(stream->buffer, "") == 0)
    {

        br=ReadFile(stream->fd, stream->buffer, BUF_SIZE, &bytes, NULL);
        if (br == 0) {
            stream->error = 1;
            return SO_EOF;
        }
        else if (bytes == 0) {
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
            br=ReadFile(stream->fd, stream->buffer, BUF_SIZE, &bytes, NULL);
            if (br == 0) {
                stream->error = 1;
                return SO_EOF;
            }
            else if (bytes == 0) {
                stream->EOFile = 1;
                return SO_EOF;
            }
                
        }
        else if (stream->PosInBuffer == bytes)
        {
            stream->EOFile = 1;
            return -1;
        }
        stream->PosInFile++;
        return stream->buffer[stream->PosInBuffer++];
    }
    stream->error = 1;
    return SO_EOF;
}

int so_fputc(int c, SO_FILE* stream)
{
    if (stream == NULL || !strcmp(stream->open, "r")) {
        printf("Operation is not valid\n*\t*\t*\n");
        if(stream!=NULL)
        stream->error = 1;
        return SO_EOF;
    }

    if (!strcmp(stream->lastOp, "read")) {
        strcpy(stream->buffer, "");
        stream->PosInBuffer = 0;
        so_fseek(stream, stream->PosInFile, SEEK_SET);
    }
    strcpy(stream->lastOp, "write");
    int pos=-1;
    if (!strcmp(stream->lastOp, "read"))
    {
        strcpy(stream->buffer, "");
        stream->PosInBuffer = 0;
    }
    if (stream->open[0] == 'a')
    {
        pos = SetFilePointer(stream->fd, 0, NULL, SEEK_END);
    }
    else if (!strcmp(stream->open, "r+"))
    {
        pos = SetFilePointer(stream->fd, stream->PosInFile, NULL, SEEK_SET);
    }
    if (-1 == pos)
    {
        stream->error = 1;
        return SO_EOF;
    }
    else {
        stream->PosInFile = pos;
    }
    strcpy(stream->lastOp, "write");
    if (stream->PosInBuffer == BUF_SIZE)
    {
        int bw=so_fflush(stream);
        if (bw == -1)
        {
            stream->error = 1;
            return SO_EOF;
        }
        strcpy(stream->buffer, "");
        stream->PosInBuffer = 0;
    }
    stream->buffer[stream->PosInBuffer++] = c;
    stream->buffer[stream->PosInBuffer] = '\0';
    stream->PosInFile++;
    return c;
}

size_t so_fread(void* ptr, size_t size, size_t nmemb, SO_FILE* stream)
{
    size_t total = size * nmemb;
    size_t i = 0;
    int  car;
    unsigned char* aux = (unsigned char*)ptr;
    while (i < total)
    {
        car = so_fgetc(stream);
        if (car != -1)
            aux[i] = (unsigned char)car;
        else {
            stream->error = 1;
            return 0;
        }
        if (stream->EOFile == 1 || stream->error == 1)
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
            stream->error = 1;
            break;
        }
        if (stream->error == 1)
        {
            break;
        }
        i++;
    }
    if (i / size != nmemb) {
        stream->error = 1;
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
    int pos = SetFilePointer(stream->fd, offset, NULL, whence);
    if (pos < 0)
        return SO_EOF;
    stream->PosInFile = pos;
    return 0;
}
long so_ftell(SO_FILE* stream)
{
    if (stream->error != 0)
    {
        return -1;
    }
    return stream->PosInFile;
}

int so_fflush(SO_FILE* stream)
{
    if (!strcmp(stream->lastOp, "write"))
    {
        DWORD nr = 0;
        int bw = 0;
        int total = strlen(stream->buffer);
        int r = 0;
        while (bw < total)
        {
            int ret = WriteFile(stream->fd, stream->buffer + bw, total - bw, &r, NULL);
            if (r == 0)
                break;
            bw += r;
        }
        if (bw == 0)
        {
            stream->error = 1;
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
HANDLE so_fileno(SO_FILE* stream)
{
    return stream->fd;
}

int so_feof(SO_FILE* stream)
{
    return stream->EOFile;
}

int so_ferror(SO_FILE* stream)
{
    return stream->error;
}

SO_FILE* so_popen(const char* command, const char* type)
{   

    SO_FILE* stream;
    HANDLE rhandle, whandle;
    SECURITY_ATTRIBUTES sec_att;
    STARTUPINFO start_info;
    PROCESS_INFORMATION proc_info;
    BOOL ret;
    char full_command[BUF_SIZE];

    strcpy(full_command, "C:\\windows\\system32\\cmd.exe /c ");
    strcat(full_command, command);

    stream = (SO_FILE*)malloc(sizeof(SO_FILE));
    if (stream == NULL)
        return NULL;

    if (strcmp(type, "r") != 0 && strcmp(type, "w") != 0) {
        free(stream);
        return NULL;
    }
    ZeroMemory(&sec_att, sizeof(sec_att));
    sec_att.nLength = sizeof(SECURITY_ATTRIBUTES);
    sec_att.bInheritHandle = TRUE;

    ZeroMemory(&proc_info, sizeof(proc_info));

    GetStartupInfo(&start_info);

    ret = CreatePipe(&rhandle, &whandle, &sec_att, 0);//cream pipe cu capatul de scriere si cel de citire
    if (ret == FALSE) {
        free(stream);
        return NULL;
    }

    if (strcmp(type, "r") == 0) {
        stream->fd = rhandle;//daca in procesul parinte citim inseamna ca handle-ul va corespunde capatului de citire
        if (whandle != INVALID_HANDLE_VALUE)
        {
            ZeroMemory(&start_info, sizeof(start_info));
            start_info.cb = sizeof(start_info);

            start_info.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
            start_info.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);;
            start_info.hStdError = GetStdHandle(STD_ERROR_HANDLE);
            start_info.dwFlags |= STARTF_USESTDHANDLES;

            start_info.hStdOutput = whandle;
            
            
        }
        ret = SetHandleInformation(rhandle, HANDLE_FLAG_INHERIT, 0);
    }
    else if (strcmp(type, "w") == 0) {
        stream->fd = whandle;
        if (rhandle != INVALID_HANDLE_VALUE) {
            ZeroMemory(&start_info, sizeof(start_info));
            start_info.cb = sizeof(start_info);

            start_info.hStdInput = rhandle;
            start_info.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
            start_info.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
            start_info.hStdError = GetStdHandle(STD_ERROR_HANDLE);
            start_info.dwFlags |= STARTF_USESTDHANDLES;

            start_info.hStdInput = rhandle;
        }
        ret = SetHandleInformation(whandle, HANDLE_FLAG_INHERIT, 0);
    }

    ret = CreateProcessA(NULL, command, NULL, NULL, TRUE, 0, NULL, NULL, &start_info, &proc_info);

    if (strcmp(type, "r") == 0)
        CloseHandle(whandle);
    else
        CloseHandle(rhandle);

    if (ret == 0) {
        free(stream);
        return NULL;
    }

    stream->child = proc_info;

    return stream;
}

int so_pclose(SO_FILE* stream) {

    int rc;
    BOOL ret;
    PROCESS_INFORMATION proc_info = stream->child;
    HANDLE fd = stream->fd;

    if (stream->PosInBuffer != 0 && strcmp(stream->lastOp,"write")==0) {
        rc = so_fflush(stream);
        if (rc <= 0) {
            free(stream);
            return rc;
        }
    }

    free(stream);

    rc = CloseHandle(fd);

    ret = WaitForSingleObject(proc_info.hProcess, INFINITE);
    if (ret == WAIT_FAILED) {
        return -1;
    }

    CloseHandle(proc_info.hProcess);
    CloseHandle(proc_info.hThread);

    return 0;
    
}
int main()
{
    SO_FILE* a = so_fopen("ana.txt", "a");
    so_fputc('6', a);
    so_fclose(a);
}



