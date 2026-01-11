#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include "common.h"
#include "parse.h"

void list_employees(struct dbheader_t *dbhdr, struct employee_t *employees) {
    for (int i = 0; i < dbhdr->count; i++) {
        printf("Employee %d\n", i);
        printf("\tName: %s\n", employees[i].name);
        printf("\tAddress: %s\n", employees[i].address);
        printf("\tHours: %d\n", employees[i].hours);
    }
}

int add_employee(struct dbheader_t *dbhdr, struct employee_t **employees, char *addstring) {
    if (NULL == dbhdr) return STATUS_ERROR;
    if (NULL == employees) return STATUS_ERROR;
    if (NULL == *employees) return STATUS_ERROR;
    if (NULL == addstring) return STATUS_ERROR;

    char *name = strtok(addstring, ",");
    if (NULL == name) return STATUS_ERROR;
    char *addr = strtok(NULL, ",");
    if (NULL == addr) return STATUS_ERROR;
    char *hours = strtok(NULL, ",");
    if (NULL == hours) return STATUS_ERROR;

    struct employee_t *e = *employees;
    e = realloc(e, sizeof(struct employee_t) * (dbhdr->count + 1));
    if (e == NULL) {
        return STATUS_ERROR;
    }
    
    dbhdr->count++;

    strncpy(e[dbhdr->count-1].name, name, sizeof(e[dbhdr->count-1].name) - 1);
    strncpy(e[dbhdr->count-1].address, addr, sizeof(e[dbhdr->count-1].address) - 1);
    e[dbhdr->count-1].hours = atoi(hours);

    *employees = e;
    return STATUS_SUCCESS;
}

int remove_employee(struct dbheader_t *dbhdr, struct employee_t **employees, char *removestring) {
    if (NULL == dbhdr) return STATUS_ERROR;
    if (NULL == employees) return STATUS_ERROR;
    if (NULL == removestring) return STATUS_ERROR;

    struct employee_t *e = *employees;

    int employeeIndexToDelete = -1;
    for (int i = 0; i < dbhdr->count; ++i) {
        if (strcmp(e[i].name, removestring) == 0) {
            employeeIndexToDelete = i;
            break;
        }
    }
    if (employeeIndexToDelete == -1) {
        printf("Employee %s not found in database\n", removestring);
        return STATUS_ERROR;
    }
    
    if (!(employeeIndexToDelete == dbhdr->count -1)) {
        for (int i = employeeIndexToDelete; i < dbhdr->count - 1; ++i) {
        e[i] = e[i + 1];
        }
    } 

    e = realloc(e, sizeof(struct employee_t) * (dbhdr->count - 1));
    if (e == NULL) {
        return STATUS_ERROR;
    } else {
        dbhdr->count--;
        *employees = e;
        return STATUS_SUCCESS;
    }
}

int modify_employee(struct dbheader_t *dbhdr, struct employee_t *employees, char *modifystring) {
    if (NULL == dbhdr) return STATUS_ERROR;
    if (NULL == employees) return STATUS_ERROR;
    if (NULL == modifystring) return STATUS_ERROR;

    char *mode = strtok(modifystring, ":");
    if (NULL == mode) return STATUS_ERROR;
    char *employee = strtok(NULL, ",");
    if (NULL == employee) return STATUS_ERROR;
    char *newValue = strtok(NULL, ",");
    if (NULL == newValue) return STATUS_ERROR;

    char modeUpper = toupper(*mode);

    for (int i = 0; i < dbhdr->count; i++) {
        if (strcmp(employees[i].name, employee) == 0) {
            switch (modeUpper) {
                case 'N':
                    strncpy(employees[i].name, newValue, sizeof(employees[i].name));
                    return STATUS_SUCCESS;
                case 'A':
                    strncpy(employees[i].address, newValue, sizeof(employees[i].address));
                    return STATUS_SUCCESS;
                case 'H':
                    employees[i].hours = atoi(newValue);
                    return STATUS_SUCCESS;
                default :
                    printf("Given mode is not a known mode value\n");
                    printf("Modes are:\n\tN: Modify name.\n\tA: Modify Address.\n\tH: Modify hours.\n");
                    return STATUS_ERROR;
            }
        }
    }
    printf("Unable to find name in Employee Database\n");
    return STATUS_ERROR;

    printf("Mode: %c, Employee to change: %s, New Value: %s\n", modeUpper, employee, newValue);
}


int read_employees(int fd, struct dbheader_t *dbhdr, struct employee_t **employeesOut) {
    if (fd < 0) {
        printf("Bad FD from the user\n");
        return STATUS_ERROR;
    }

    int count = dbhdr->count;

    struct employee_t *employees = calloc(count, sizeof(struct employee_t));
    if (employees == NULL) {
        printf("Malloc failed\n");
        return STATUS_ERROR;
    }

    read(fd, employees, count*sizeof(struct employee_t));

    for (int i = 0; i < count; i++) {
        employees[i].hours = ntohl(employees[i].hours);
    }

    *employeesOut = employees;
    return STATUS_SUCCESS;
}

int output_file(int fd, struct dbheader_t *dbhdr, struct employee_t *employees) {
    if (fd < 0) {
        printf("Bad FD from the user\n");
        return STATUS_ERROR;
    }

    int realcount = dbhdr->count;

    dbhdr->magic = htonl(dbhdr->magic);
    dbhdr->filesize = htonl(sizeof(struct dbheader_t) + (sizeof(struct employee_t) * realcount));
    dbhdr->count = htons(dbhdr->count);
    dbhdr->version = htons(dbhdr->version);

    lseek(fd, 0, SEEK_SET);

    ftruncate(fd, sizeof(struct dbheader_t) + (sizeof(struct employee_t) * realcount));

    write(fd, dbhdr, sizeof(struct dbheader_t));

    for (int i = 0; i < realcount; i++ ) {
        employees[i].hours = htonl(employees[i].hours);
        write(fd, &employees[i], sizeof(struct employee_t));
    }

    return 0;
}	

int validate_db_header(int fd, struct dbheader_t **headerOut) {
    if (fd < 0) {
        printf("Bad FD from the user\n");
        return STATUS_ERROR;
    }

    struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
    if (header == NULL) {
        printf("Malloc failed to create a db header\n");
        return STATUS_ERROR;
    }

    if (read(fd, header, sizeof(struct dbheader_t)) != sizeof(struct dbheader_t)) {
        perror("read");
        free(header);
        return STATUS_ERROR;
    }

    header->version = ntohs(header->version);
    header->count = ntohs(header->count);
    header->magic = ntohl(header->magic);
    header->filesize = ntohl(header->filesize);

    if (header->magic != HEADER_MAGIC) {
        printf("Improper header magic\n");
        free(header);
        return -1;
    }

    if (header->version != 1) {
        printf("Improper header version\n");
        free(header);
        return -1;
    }

    struct stat dbstat = {0};
    fstat(fd, &dbstat);
    if (header->filesize != dbstat.st_size) {
        printf("Corrupted database\n");
        free(header);
        return -1;
    }

    *headerOut = header;
    return STATUS_SUCCESS;

}

int create_db_header(int fd, struct dbheader_t **headerOut) {
	struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
    if (header == NULL) {
        printf("Malloc failed to create db header\n");
        return STATUS_ERROR;
    }

    header->version = 0x1;
    header->count = 0;
    header->magic = HEADER_MAGIC;
    header->filesize = sizeof(struct dbheader_t);

    *headerOut = header;

    return STATUS_SUCCESS;
}


