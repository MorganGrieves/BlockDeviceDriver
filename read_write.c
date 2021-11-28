#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>

#include "mybdev_ioctl.h"

#define BUFFER_SIZE 64

int open_device(char *pathname)
{
        int device_fd = open(pathname, O_RDWR);
        if (device_fd < 0) {
                printf("Cannot open file %s\n", pathname);
                printf("Error no - %d\n", errno);
                printf("Error string - %s\n", strerror(errno));
                return -1;
        }

        return device_fd;
}

int write_and_close(char *pathname, char *buffer)
{
        int status = 0;
        int device_fd;

        if ((device_fd = open_device(pathname)) < 0)
                return -1;

        status = write(device_fd, buffer, BUFFER_SIZE);
        if (status < 0) {
                printf("Cannot write: file_descriptor = %s\n", device_fd);
                printf("Error no - %d\n", errno);
                printf("Error string - %s\n", strerror(errno));
                close(device_fd);

                return -1;
        }
        close(device_fd);

        return 0;
}

int read_and_print(char * pathname)
{
        int status = 0;
        char buffer[BUFFER_SIZE] = "";
        int device_fd;

        if ((device_fd = open_device(pathname)) < 0)
                return -1;

        status = read(device_fd, buffer, BUFFER_SIZE);
        if (status < 0) {
                printf("Cannot read %s\n", pathname);
                printf("Error no - %d\n", errno);
                printf("Error string - %s\n", strerror(errno));
                close(device_fd);
                return -1;
        }
        
        close(device_fd);
        printf("Device data = %s\n", buffer);
        return 0;
}

int set_read_only(char * pathname)
{
        int rd_only = 1;
        int device_fd;

        if ((device_fd = open_device(pathname)) < 0)
                return -1;

        if (ioctl(device_fd, BLK_READ_ONLY, &rd_only) < 0) {
                printf("Error ioctl\n");
                printf ("Error no - %d\n", errno);
                printf ("Error string - %s\n", strerror(errno));
                close(device_fd);
                return -1;
        }

        return 0;
}

int main (int argc, char ** argv)
{
        char buffer[BUFFER_SIZE] = "just some random text hello ollo mollo";
        char *device_pathname = argv[1];

        if (argc < 2) {
                printf("Too few arguments\n");
                exit(1);
        }

        //write to - must work
        if (write_and_close(device_pathname, buffer) < 0)
                exit(1);

        //read from - must work
        if (read_and_print(device_pathname) < 0)
                exit(1);

        //set read only value
        if (set_read_only(device_pathname) < 0)
                exit(1);

        //write to - must not work
        if (write_and_close(device_pathname, buffer) < 0)
                exit(1);

        //read from - must work
        if (read_and_print(device_pathname) < 0)
                exit(1);

        exit (0);
}
