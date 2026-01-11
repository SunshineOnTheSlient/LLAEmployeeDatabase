#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>

#include "common.h"
#include "file.h"
#include "parse.h"

void print_usage(char *argv[]) {
    printf("Usage: %s -n -f <database file>\n", argv[0]);
    printf("\t -n - create new database file\n");
    printf("\t -f - (required) path to database file\n");
    printf("\t -l - list all employees in database file\n");
    printf("\t -a - adds employee to database file via CSV list (name,address,hours)\n");
    printf("\t -r - removes employee from database file via name\n");
    printf("\t -m - Modifies an employee in the database using the follow format (MODE:EmployeeName,NewValue) Where Mode is one of the following:\n");
    printf("\t\tN: Modify name.\n\t\tA: Modify Address.\n\t\tH: Modify hours.\n");
    return;
}

int main(int argc, char *argv[]) { 
	
    char *filepath = NULL;
    char *addstring = NULL;
    char *removestring = NULL;
    char *modifystring = NULL;
    bool newfile = false;
    bool list = false;
    int c;

    int dbfd = -1;
    struct dbheader_t *dbhdr = NULL;
    struct employee_t *employees = NULL;

    while ((c = getopt(argc, argv, "nf:a:lr:m:")) != -1) {
        switch (c) {
            case 'n' :
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
                removestring = optarg;
                break;
            case 'm':
                modifystring = optarg;
                break;
            case '?' :
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
                printf("Failed to validate Database header\n");
                return -1;
            }
    }


    if (read_employees(dbfd, dbhdr, &employees) != STATUS_SUCCESS) {
        printf("failed to read employees");
        return 0;
    }


    if (addstring) {
        add_employee(dbhdr, &employees, addstring);
    }

    if (list) {
        list_employees(dbhdr, employees);
    }

    if (removestring) {
        if (remove_employee(dbhdr, &employees, removestring) == STATUS_ERROR) {
            printf("unable to remove employee from database\n");
            return -1;
        }
    }

    if (modifystring) {
        if (modify_employee(dbhdr, employees, modifystring) == STATUS_ERROR) {
            printf("Unable to modify employee in database\n");
            return -1;
        }
    }

    output_file(dbfd, dbhdr, employees);

    //clean up
    free(employees);
    free(dbhdr);
    close(dbfd);

    return 0;
}
