#include <stdio.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "file.h"

int create_db_file(char *filename) {
  int fd = open(filename, O_RDONLY);
  if (fd != -1) {
    close(fd);
    printf("File already exists\n");
    return STATUS_ERROR;
  }

  fd = open(filename, O_RDWR | O_CREAT, 0644); // 0 octal
  if (fd == -1) {
    perror("open");
    printf("Error in file.c create_db_file \n");
    return STATUS_ERROR;
  }

  return fd;
}

int open_db_file(char *filename) {
  int fd = open(filename, O_RDWR | O_CREAT, 0644);
  if (fd == -1) {
    perror("open");
    printf("Error in file.c open_db_file \n");
    return STATUS_ERROR;
  }

  return fd;
}

int trunc_db_file(char *filename) {
  int fd = open(filename, O_RDWR | O_TRUNC, 0644);
  if (fd == -1) {
    perror("open");
    printf("Error in file.c trunc_db_file \n");
    return STATUS_ERROR;
  }

  return fd;
}
