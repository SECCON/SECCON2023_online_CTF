#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#define IOCTL_MAGIC 'S'
#define MEMO_STORE  _IOR(IOCTL_MAGIC, 0, uint32_t)
#define MEMO_LOAD   _IOW(IOCTL_MAGIC, 1, uint32_t)

static void memo(char **memos);
static void free_space(int fd);
static ssize_t getnline(char *buf, size_t size);
static int getint(void);

__attribute__((constructor))
static int init(){
	setbuf(stdin, NULL);
	setbuf(stdout, NULL);
	return 0;
}

int main(int argc, char *argv[]){
	int fd;

	if(argc < 2){
		printf("Usage: %s <file path> [key]\n", argv[0]);
		return -1;
	}

	if((fd = open(argv[1], O_RDWR)) < 0){
		perror("open");
		return -1;
	}

	if(argc > 2){
		uint32_t key;
		sscanf(argv[2], "%x", &key);
		if(ioctl(fd, MEMO_LOAD, key))
			perror("ioctl");
		else
			puts("Successfully restored");
	}

	char **memos;
	if((memos = mmap(NULL, 0x1000, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED){
		perror("mmap");
		return -1;
	}

	for(int i=0; i<0xf; i++)
		memos[i] = (char*)memos + 0x100*(i+1);

	for(;;){
		printf("\nMENU\n"
				"1. Use fixed form memo\n"
				"2. Use free space\n"
				// "3. Save\n"
				"0. Exit\n"
				"> ");

		switch(getint()){
			case 1:
				memo(memos);
				break;
			case 2:
				free_space(fd);
				break;
			/*
			case 3:
				{
				uint32_t key;
				if(ioctl(fd, MEMO_STORE, &key))
					perror("ioctl");
				else
					printf("Saved (key:%08x)\n", key);
				}
				break;
			 */
			default:
				goto end;
		}
	}

end:
	munmap(memos, 0x1000);
	close(fd);

	puts("Bye.");
	return 0;
}

static void memo(char **memos){
	unsigned idx;

	int get_idx(void){
		printf("Index: ");
		if((idx = getint()) > 0xe){
			puts("Out of range");
			return -1;
		}
		return 0;
	}

	for(;;){
		printf("\n"
				"1. Read\n"
				"2. Write\n"
				"0. Back\n"
				"M> ");

		switch(getint()){
			case 1:
				if(get_idx() < 0)
					break;

				printf("Output: ");
				if(write(STDOUT_FILENO, memos[idx], 0x100) < 0)
					puts("Read memo failed...");
				break;
			case 2:
				if(get_idx() < 0)
					break;

				printf("Input: ");
				if(read(STDIN_FILENO, memos[idx], 0x100) < 0)
					puts("Write memo failed...");
				break;
			default:
				return;
		}
	}
}

static void free_space(int fd){
	size_t len;
	char buf[0x400];

	int get_ofs_sz(void){
		uint32_t offset;

		printf("Offset: ");
		if((offset = getint() + 0x1000) < 0x1000 || lseek(fd, offset, SEEK_SET) < 0){
			puts("Out of range");
			return -1;
		}

		printf("Size: ");
		if((len = getint()) > 0x400){
			puts("Too large");
			return -1;
		}

		return 0;
	}

	for(;;){
		printf("\n"
				"1. Read\n"
				"2. Write\n"
				"0. Back\n"
				"S> ");

		switch(getint()){
			case 1:
				if(get_ofs_sz() < 0)
					break;

				printf("Output: ");
				if((ssize_t)(len = read(fd, buf, len)) < 0 || write(STDOUT_FILENO, buf, len) < 0)
					puts("Read space failed...");
				break;
			case 2:
				if(get_ofs_sz() < 0)
					break;

				printf("Input: ");
				if((ssize_t)(len = read(STDIN_FILENO, buf, len)) < 0 || write(fd, buf, len) < 0)
					puts("Write space failed...");
				break;
			default:
				return;
		}
	}
}

static ssize_t getnline(char *buf, size_t size){
	ssize_t len;

	bzero(buf, size);

	if(!size || (len = read(STDIN_FILENO, buf, size-1)) <= 0)
		return -1;

	if(buf[len-1]=='\n')
		len--;
	buf[len] = '\0';

	return len;
}

static int getint(void){
	char buf[0x10] = {};

	getnline(buf, sizeof(buf));
	return atoi(buf);
}
