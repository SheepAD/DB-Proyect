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



int remove_employee(struct dbheader_t *dbhdr, struct employee_t **employees, char *whom){
	if(!dbhdr || !employees || !whom) return STATUS_ERROR;

	int idx = -1;
	int count = dbhdr->count;
	if (count == 0) return STATUS_ERROR;
	if (*employees == NULL) return STATUS_ERROR;
	for(int i = 0; i < count; i++){
		if(strcmp((*employees)[i].name, whom) == 0){
			idx = i;
			break;
		}
	}


	if (idx == -1){
		printf("[!] User %s NOT found :( \n", whom);
		return STATUS_ERROR;
	}

	if (idx < count - 1) {
    	memmove(&(*employees)[idx],
        	    &(*employees)[idx + 1],
                (size_t)(count - idx - 1) * sizeof((*employees)[0]));
    }

	dbhdr->count = count - 1;

	if(dbhdr->count == 0){
		free(*employees);
		*employees = NULL;
	} else {
		struct employee_t *tmp =  realloc(*employees, (size_t)dbhdr->count * sizeof((*employees)[0]));
		if(!tmp) return STATUS_ERROR;
		*employees = tmp;		
	}



	return STATUS_SUCCESS;
}

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
    if (count == 0) {
        *employeesOut = NULL;
        return STATUS_SUCCESS;
    }

    struct employee_t *employees = calloc((size_t)count, sizeof(*employees));
    if(!employees) return STATUS_ERROR;

    if (read(fd, employees, (size_t)count * sizeof(*employees)) != (ssize_t)((size_t)count * sizeof(*employees))) {
        free(employees);
        return STATUS_ERROR;
    }

    for(int i = 0; i < count; i++){
        employees[i].hours = ntohl(employees[i].hours);
    }

    *employeesOut = employees;
    return STATUS_SUCCESS;
}



int output_file(int fd, struct dbheader_t *dbhdr, struct employee_t *employees) {
    if (fd < 0 || !dbhdr) return STATUS_ERROR;

    // dbhdr está en host order SIEMPRE
    uint16_t count = dbhdr->count;

    // header copy en network order
    struct dbheader_t out = *dbhdr;
    out.magic    = htonl(out.magic);
    out.version  = htons(out.version);
    out.count    = htons(out.count);

    uint32_t new_size = (uint32_t)(sizeof(struct dbheader_t) + sizeof(struct employee_t) * count);
    out.filesize = htonl(new_size);

    // rebobinar
    if (lseek(fd, 0, SEEK_SET) == (off_t)-1) return STATUS_ERROR;

    // escribir header
    if (write(fd, &out, sizeof(out)) != (ssize_t)sizeof(out)) return STATUS_ERROR;

    // escribir empleados (con hours en network order, pero sin mutar el array original)
    for (uint16_t i = 0; i < count; i++) {
        struct employee_t e = employees[i];
        e.hours = htonl(e.hours);
        if (write(fd, &e, sizeof(e)) != (ssize_t)sizeof(e)) return STATUS_ERROR;
    }

    // IMPORTANTÍSIMO: truncar el archivo al tamaño nuevo
    if (ftruncate(fd, (off_t)new_size) == -1) return STATUS_ERROR;

    return STATUS_SUCCESS;
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

