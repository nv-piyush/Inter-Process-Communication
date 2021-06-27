#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <memory.h>
#include <stdlib.h>


char const * storage_filepath = "shared_check";
int file_length = 512;

int main(int argc, char* argv[]) {
	shm_unlink(storage_filepath); // unlink shared memory from before if we failed and didnt unlink
	pid_t process_pid = getpid();
	int memory_fd = shm_open(
		"shared_check",
		O_RDWR | O_CREAT,
		S_IRUSR | S_IWUSR
	);
	if (memory_fd == -1) {
		fprintf(stderr, "Error with shm_open\n");
		fprintf(stderr, "%s\n", strerror(errno));
	}
	// extend memory size by file_length bytes
	int extend = ftruncate(memory_fd, file_length);
	if (extend == -1) {
		fprintf(stderr, "ftruncate() failed\n");
	}
	void* memory_address = mmap(
		NULL,
		file_length,
		PROT_WRITE | PROT_READ,
		MAP_SHARED,
		memory_fd,
		0
	);
	if (memory_address == MAP_FAILED) fprintf(stderr, "map failed\n");

	// write data to shared memory
	char* data = (char *) malloc(100 * sizeof(char));
	
	fprintf(stderr, "In main process\n");
	
	sprintf(data, "Written data, Process ID: %d\n", process_pid);
	memcpy(memory_address, data, strlen(data));
	
	// create a separate process
	int fork_ret = fork();
	if (fork_ret == -1) fprintf(stderr, "fork() failed\n");
	if (fork_ret == 0) {
		//Child process - read operation
		char * read_data = (char *) malloc(100 * sizeof(char));
		memcpy(read_data, memory_address, strlen(data));
		
		fprintf(stderr, "In child, read: %s\n", read_data);
		free(data);
	} else {
		// wait for child process to read
		pid_t child_pid = wait(0);

		// clean up, unmap the mmaped addresses and unlink shared memory.
		munmap(memory_address, file_length);
		shm_unlink(storage_filepath);
		free(data);
	}
	return 0;
}