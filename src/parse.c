#include <arpa/inet.h>
#include <asm-generic/errno-base.h>
#include <float.h>
#include <iso646.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "parse.h"

void list_employees(struct dbheader_t *dbhdr, struct employee_t *employees) {
  int i;
  if (dbhdr->count == 0) {
    printf("DB is empty\n");
  }
  for (i = 0; i < dbhdr->count; i++) {
    printf("Employee %d\n", i + 1);
    printf("\tID: %u\n", employees[i].id);
    printf("\tName: %s\n", employees[i].name);
    printf("\tAddress: %s\n", employees[i].address);
    printf("\tHours: %u\n", employees[i].hours);
  }
}

int add_employee(struct dbheader_t *dbhdr, struct employee_t *employees,
                 char *addstring) {
  char *name = strtok(addstring, ",");
  char *addr = strtok(NULL, ",");
  char *hours = strtok(NULL, ",");

  strncpy(employees[dbhdr->count - 1].name, name,
          sizeof(employees[dbhdr->count - 1].name));
  strncpy(employees[dbhdr->count - 1].address, addr,
          sizeof(employees[dbhdr->count - 1].address));

  employees[dbhdr->count - 1].hours = atoi(hours);
  get_id(dbhdr, employees);

  return STATUS_SUCCESS;
}

void get_id(struct dbheader_t *dbhdr, struct employee_t *employees) {
  if (dbhdr->count - 1 == 0) {
    employees[dbhdr->count - 1].id = 1;
    return;
  }

  unsigned int i;
  unsigned int oldEmpCount = dbhdr->count - 1;
  unsigned int actarr[dbhdr->count - 1];
  unsigned int maxID = 0;
  unsigned int newID = 0;
  bool existingID;

  for (newID = 1; newID <= oldEmpCount; newID++) {
    for (i = 0; i < oldEmpCount; i++) {
      existingID = (employees[i].id == newID) ? true : false;
      if (existingID) {
        break;
      }
    }

    if (newID == oldEmpCount) {
      employees[dbhdr->count - 1].id = dbhdr->count;
      break;
    }
    if (!existingID) {
      employees[oldEmpCount].id = newID;
      break;
    }
  }

  return;
}

int remove_employee(struct dbheader_t *dbhdr, struct employee_t **employees,
                    char *id) {
  bool idfound = false;
  int i = 0;
  unsigned int targetid = atoi(id);

  if (targetid == 0) {
    printf("Target id should be numeric and higher than 0\n");
    return STATUS_ERROR;
  }

  if (dbhdr->count - 1 == 0) {
    free(*employees);
    *employees = NULL;
    printf("Employees database is emptied\n");
    return STATUS_SUCCESS;
  }

  for (i; i < dbhdr->count; i++) {
    if ((*employees)[i].id == targetid) {
      printf("Removed employee %s, with ID %u\n", (*employees)[i].name,
             (*employees)[i].id);

      dbhdr->count--;
      memmove(&(*employees)[i], &(*employees)[i + 1],
              (dbhdr->count - i) * sizeof(struct employee_t));

      struct employee_t *temployees =
          realloc(*employees, dbhdr->count * sizeof(struct employee_t));
      if (temployees != NULL) {
        *employees = temployees;
      } else {
        printf("Realloc failure\n");
        return STATUS_ERROR;
      }

      idfound = true;
      break;
    }
  }

  if (!idfound) {
    return STATUS_ERROR;
  }

  return STATUS_SUCCESS;
}

int update_hours(struct dbheader_t *dbhdr, struct employee_t **employees,
                 char *updatestring) {
  bool idfound = false;
  int i = 0;
  unsigned int targetid = atoi(strtok(updatestring, ","));
  char *hours = strtok(updatestring, ",");

  if (targetid == 0) {
    printf("Target id should be numeric and higher than 0\n");
    return STATUS_ERROR;
  }

  for (i; i < dbhdr->count; i++) {
    if ((*employees)[i].id == targetid) {
      (*employees)[i].hours = atoi(hours);

      printf("Updated employee %s, with ID %u\n", (*employees)[i].name,
             (*employees)[i].id);

      idfound = true;
      break;
    }
  }

  if (!idfound) {
    return STATUS_ERROR;
  }

  return STATUS_SUCCESS;
}

int read_employees(int fd, struct dbheader_t *dbhdr,
                   struct employee_t **employeesOut) {
  if (fd < 0) {
    printf("Got a bad FD from the user\n");
    return STATUS_ERROR;
  }

  int count = dbhdr->count;

  struct employee_t *employees = calloc(count, sizeof(struct employee_t));
  if (employees == -1) {
    printf("Malloc failed \n");
    return STATUS_ERROR;
  }

  read(fd, employees, count * sizeof(struct employee_t));

  int i;
  for (i = 0; i < count; i++) {
    employees[i].hours = ntohl(employees[i].hours);
  }

  *employeesOut = employees;

  return STATUS_SUCCESS;
}

int output_file(int fd, struct dbheader_t *dbhdr,
                struct employee_t *employees) {
  if (fd < 0) {
    printf("Got a bad FD from the user\n");
    return STATUS_ERROR;
  }

  int realcount = dbhdr->count;
  //  printf("realcount = %u\n", realcount);
  // printf("headermagic: %u\n", dbhdr->magic);

  // printf("dbheader_t size = %u\n", sizeof(struct dbheader_t));
  //  printf("employee_t size = %u\n", sizeof(struct employee_t));

  dbhdr->magic = htonl(dbhdr->magic);
  dbhdr->filesize =
      htonl(sizeof(struct dbheader_t) + sizeof(struct employee_t) * realcount);
  dbhdr->count = htons(dbhdr->count);
  dbhdr->version = htons(dbhdr->version);

  // printf("filesize = %u\n", dbhdr->filesize);

  lseek(fd, 0, SEEK_SET);

  write(fd, dbhdr, sizeof(struct dbheader_t));

  int i;
  for (i = 0; i < realcount; i++) {
    employees[i].hours = htonl(employees[i].hours);
    write(fd, &employees[i], sizeof(struct employee_t));
  }

  return 0;
}

int validate_db_header(int fd, struct dbheader_t **headerOut) {
  if (fd < 0) {
    printf("Got a bad FD from the user\n");
    return STATUS_ERROR;
  }

  struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));

  if (header == -1) {
    printf("Calloc failed to create db header\n");
    return STATUS_ERROR;
  }

  ssize_t headerSize = read(fd, header, sizeof(struct dbheader_t));
  if (headerSize != sizeof(struct dbheader_t)) {
    perror("read");
    free(header);
    return STATUS_ERROR;
  }

  header->version = ntohs(header->version);
  header->count = ntohs(header->count);
  header->magic = ntohl(header->magic);
  header->filesize = ntohl(header->filesize);

  if (header->version != 1) {
    printf("Improper header version\n");
    free(header);
    return -1;
  }

  if (header->magic != HEADER_MAGIC) {
    printf("Improper header magic\n");
    free(header);
    return -1;
  }

  struct stat dbstat = {0};
  fstat(fd, &dbstat);
  if (header->filesize != dbstat.st_size) {
    printf("Corrupted database, reason filesize mismatch\n");
    printf("Expected filesize : %u\n", header->filesize);
    printf("Actual filesize : %u\n", dbstat.st_size);

    free(header);
    return -1;
  }

  *headerOut = header;

  return 0;
}
int create_db_header(int fd, struct dbheader_t **headerOut) {
  struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
  if (header == -1) {
    printf("Malloc failed to create db header\n");
    return STATUS_ERROR;
  }

  // access structure use .
  // access pointer structure use ->
  header->version = 0x1;
  header->count = 0;
  header->magic = HEADER_MAGIC;
  header->filesize = sizeof(struct dbheader_t);

  *headerOut = header;
  return STATUS_SUCCESS;
}
