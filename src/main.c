#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include <stdlib.h>

#include "common.h"
#include "file.h"
#include "parse.h"

void print_usage(char *argv[]) {
	printf("[?] Usage: %s -n -f <database file>\n", argv[0]);
	printf("\t -n  -  Create a new database file\n");
	printf("\t -f  -  (required) - Path to database file\n");
	printf("\t -a  -  Add employees to database file (name,address,salary)\n");
	printf("\t -l  -  list employees in the database file\n");
	printf("\t -r  -  remove employee from the database file\n");

}

int main(int argc, char *argv[]) { 
	int c = 0;
	bool newfile = false;
	char *filepath = NULL;
	char *addstring = NULL;
	bool list = false;
	char *whom = NULL;

	int dbfd = -1;
	struct dbheader_t *dbhdr = NULL;
	struct employee_t *employees = NULL; 

	while ((c = getopt(argc, argv, "nf:a:lr:")) != -1){
		switch(c){
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
				whom = optarg;
				break;
			case '?':
				printf("[!] Unkown Option: -> -%c\n\n", optopt);
				print_usage(argv);
				break;
			default:
				return -1;

		}
	}

	if(!filepath){
		printf("[!] Filepath is a required argument\n");
		print_usage(argv);
		return -1;
	}
	if(newfile){
			dbfd = create_db_file(filepath);
			if(dbfd == STATUS_ERROR){
				printf("[!] Unable to create DB file\n");
				return -1;
			} 

			if(create_db_header(&dbhdr) == STATUS_ERROR){
				printf("[!] Failed to create DB header\n");
				return -1;
			}

	} else {
			dbfd = open_db_file(filepath);
			if(dbfd == STATUS_ERROR){
				printf("[!] Unable to open DB file\n");
				return -1;
			}

			if(validate_db_header(dbfd, &dbhdr) == STATUS_ERROR){
				printf("[!] Failed to validate DB header\n");
				return -1;
			}
	}

	printf("Newfile: %d\n", newfile);
	printf("Filepath: %s\n", filepath);

	if(read_employees(dbfd, dbhdr, &employees) != STATUS_SUCCESS){
		printf("[!] Filed to read employees\n");
		return -1;
	}

	if(addstring){
		add_employee(dbhdr, &employees, addstring);
	}

	if(list){
		list_employees(dbhdr, employees);
	}

	if(whom){
		remove_employee(dbhdr, &employees, whom);
	}


	output_file(dbfd, dbhdr, employees);

	return 0;
}
