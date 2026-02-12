#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "common.h"
#include "parse.h"


void list_employees(struct dbheader_t *dbhdr, struct employee_t *employees){
	if (!dbhdr || !employees) return;

	for(int i = 0; i < dbhdr->count; i++){
		printf("[+] Employee %d\n", i);
		printf("\tName: %s\n", employees[i].name);
		printf("\tAddress: %s\n", employees[i].address);
		printf("\tHours: %d\n", employees[i].hours);
	}
}

int add_employee(struct dbheader_t *dbhdr, struct employee_t **employees, char *addstring){
    if (!dbhdr || !employees || !addstring) return STATUS_ERROR;

    char *name  = strtok(addstring, ",");
    char *addr  = strtok(NULL, ",");
    char *hours = strtok(NULL, ",");

    if (!name || !addr || !hours) return STATUS_ERROR;

    int old = dbhdr->count;

    struct employee_t *tmp = realloc(*employees, (old + 1) * sizeof(**employees));
    if (!tmp) return STATUS_ERROR;

    *employees = tmp;
    dbhdr->count = old + 1;
    int idx = old;

    strncpy((*employees)[idx].name, name, sizeof((*employees)[idx].name) - 1);
    (*employees)[idx].name[sizeof((*employees)[idx].name) - 1] = '\0';

    strncpy((*employees)[idx].address, addr, sizeof((*employees)[idx].address) - 1);
    (*employees)[idx].address[sizeof((*employees)[idx].address) - 1] = '\0';

    (*employees)[idx].hours = (unsigned int)atoi(hours);

    return STATUS_SUCCESS;
}


int read_employees(int fd, struct dbheader_t *dbhdr, struct employee_t **employeesOut){
	if(fd < 0 || !dbhdr || !employeesOut) return STATUS_ERROR;

	int count = dbhdr->count;
	struct employee_t *employees = calloc(count, sizeof(struct employee_t));
	if(!employees) return STATUS_ERROR;

	read(fd, employees, count*sizeof(struct employee_t));
	for(int i = 0; i < count; i++){
		employees[i].hours = ntohl(employees[i].hours);
	}

	*employeesOut = employees;
	return STATUS_SUCCESS;
}





int output_file(int fd, struct dbheader_t *dbhdr, struct employee_t *employees) {
    
    if(fd < 0 || !dbhdr) return STATUS_ERROR;

    int count = dbhdr->count;

    dbhdr->magic = htonl(dbhdr->magic);
	dbhdr->filesize = htonl(sizeof(struct dbheader_t) + (sizeof(struct employee_t) * count));
	dbhdr->count = htons(dbhdr->count);
    dbhdr->version = htons(dbhdr->version);

    lseek(fd, 0, SEEK_SET);
    write(fd, dbhdr, sizeof(struct dbheader_t));
    
    for(int i = 0; i < count; i++){
    	employees[i].hours = htonl(employees[i].hours);
    	write(fd, &employees[i], sizeof(struct employee_t));
    }

    return 0;
}


int validate_db_header(int fd, struct dbheader_t **headerOut){
	if(fd < 0) return STATUS_ERROR;

	struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
	if(!header) return STATUS_ERROR;

	if (read(fd, header, sizeof(struct dbheader_t)) != sizeof(struct dbheader_t)){
		perror("read");
		free(header);
		return STATUS_ERROR;
	}

	header->version = ntohs(header->version);
	header->count = ntohs(header->count);
	header->magic = ntohl(header->magic);
	header->filesize = ntohl(header->filesize);

	if(header->version != 1){
		printf("Improper version\n");
		free(header);
		return STATUS_ERROR;
	}

	if(header->magic != HEADER_MAGIC){
		printf("Improper header magic\n");
		free(header);
		return STATUS_ERROR;
	}

	struct stat dbstat = {0};
	fstat(fd,&dbstat);
	if(header->filesize != dbstat.st_size){
		printf("Corrumped DB file\n");
		free(header);
		return STATUS_ERROR;
	}

	*headerOut = header;
	return STATUS_SUCCESS;
}


int create_db_header(struct dbheader_t **headerOut) {
	if(!headerOut) return STATUS_ERROR;
	struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
	if(!header) return STATUS_ERROR;

	header->magic = HEADER_MAGIC;
	header->version = 0x1;
	header->count = 0;
	header->filesize = sizeof(struct dbheader_t);

	*headerOut = header;

	return STATUS_SUCCESS;
}

