#include <endian.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "common.h"
#include "file.h"
#include "parse.h"

void print_usage(char *argv[]) {
  printf(
      "Usage: %s -f <database file> -n -l -a <new employee -r <employee id> -u "
      "<employee id, hours>\n",
      argv[0]);
  printf("\t -f - (required path to database file\n");
  printf("\t -n - create new database file\n");
  printf("\t -l - list all employees in database\n");
  printf("\t -a - add employee to database\n");
  printf("\t -r - remove employee from database\n");
  printf("\t -u - update employee hours\n");
  return;
}

int main(int argc, char *argv[]) {
  char *filepath = NULL;
  char *addstring = NULL;
  char *remstring = NULL;
  char *upstring = NULL;
  bool newfile = false;
  bool list = false;
  int c;

  int dbfd = -1; // database filedescriptor
  struct dbheader_t *dbhdr = NULL;
  struct employee_t *employees = NULL;

  while ((c = getopt(argc, argv, "nf:a:lr:u:")) !=
         -1) { //: means that it has data
    switch (c) {
    case 'n':
      newfile = true;
      break;
    case 'f':
      filepath = optarg;
      break;
    case 'a':
      addstring = optarg;
      break;
    case 'l':
      list = true;
      break;
    case 'r':
      remstring = optarg;
      break;
    case 'u':
      upstring = optarg;
      break;
    case '?':
      printf("Unkown option -%c\n", c);
      break;
    default:
      return -1;
    }
  }

  if (filepath == NULL) {
    printf("Filepath is a required argument\n");
    print_usage(argv);
    return 0;
  }

  if (newfile) {
    dbfd = create_db_file(filepath);
    if (dbfd == STATUS_ERROR) {
      printf("Unable to create database file\n");
      return -1;
    }

    if (create_db_header(dbfd, &dbhdr) == STATUS_ERROR) {
      printf("Failed to create database header\n");
      return -1;
    }
  } else {
    dbfd = open_db_file(filepath);
    if (dbfd == STATUS_ERROR) {
      printf("Unable to open database file\n");
      return -1;
    }

    if (validate_db_header(dbfd, &dbhdr) == STATUS_ERROR) {
      printf("Failed to validate database header\n");
      return -1;
    }
  }

  if (read_employees(dbfd, dbhdr, &employees) != STATUS_SUCCESS) {
    printf("Failed to read employees");
    return 0;
  }

  if (addstring) {
    dbhdr->count++;
    employees = realloc(employees, dbhdr->count * (sizeof(struct employee_t)));
    add_employee(dbhdr, employees, addstring);
  }

  if (list) {
    list_employees(dbhdr, employees);
  }

  if (remstring) {
    if (remove_employee(dbhdr, &employees, remstring) == STATUS_ERROR) {
      if (dbhdr->count <= 0) {
        printf("Failed remove: database empty\n");
      } else {
        printf("Failed remove: couldn't find id\n");
      }
      return -1;
    }
    dbfd = trunc_db_file(filepath);
  }

  if (upstring) {
    if (update_hours(dbhdr, employees, upstring) == STATUS_ERROR) {
      if (dbhdr->count <= 0) {
        printf("Failed update hours: database empty\n");
      } else {
        printf("Failed update hours: couldn't find id\n");
      }
    }
  }

  //  printf("Newfile: %d\n", newfile);
  //  printf("Filepath: %s\n", filepath);

  output_file(dbfd, dbhdr, employees);

  return 0;
}
