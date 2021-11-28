#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>	/* read(), write(), close() */
#include <fcntl.h>	/* open(), O_RDONLY */
#include <sys/stat.h>	/* S_IRUSR */
#include <sys/types.h>	/* mode_t */
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>

#include "mybdev_ioctl.h"

#define BUFFER_SIZE 64

int main (int argc, char ** argv)
{
    int fd, fd1;
    ssize_t read_bytes;
    ssize_t written_bytes;
    char buffer[BUFFER_SIZE] = "hello people";
    
    if (argc < 3)
    {
        printf("Too few arguments\n");
        exit (1);
    }

//     fd = open (argv[1], O_RDWR);
//     if (fd < 0)
//     {
//         printf ("Cannot open file %s\n", argv[1]);
//         printf ("Error no - %d\n", errno);
//         printf ("Error string - %s\n", strerror(errno));
//         exit (1);
//     }
// 
//     fd1 = open(argv[2], O_RDWR);
//     if (fd1 < 0)
//     {
//         printf ("Cannot open file %s\n", argv[2]);
//         printf ("Error no - %d\n", errno);
//         printf ("Error string - %s\n", strerror(errno));
//         close(fd);
//         exit (1);
//     }
//     
//     while ((read_bytes = read (fd1, buffer, BUFFER_SIZE)) > 0)
//     {
//         written_bytes = write (fd, buffer, read_bytes);
//         if (written_bytes != read_bytes)
//         {
//             printf ("Cannot write\n");
//             printf ("Error no - %d\n", errno);
//             printf ("Error description - %s\n", strerror(errno));
//             close(fd1);
//             close(fd);
//             exit (1);
//         }
//     }
//     
//     if (read_bytes < 0)
//     {
//         printf("myread: Cannot read file\n");
//         close (fd);
//         close(fd1);
//         exit (1);
//     }
//     fd = open (argv[1], O_RDWR);
//     if (fd < 0)
//     {
//         printf ("Cannot open file %s\n", argv[1]);
//         printf ("Error no - %d\n", errno);
//         printf ("Error string - %s\n", strerror(errno));
//         exit (1);
//     }
// 
    fd = open(argv[1], O_RDWR);
    if (fd1 < 0)
    {
        printf ("Cannot open file %s\n", argv[2]);
        printf ("Error no - %d\n", errno);
        printf ("Error string - %s\n", strerror(errno));
        close(fd);
        exit (1);
    }
    
    int value = 0;
    
//     if (ioctl(fd, BLK_READ_ONLY, &value) < 0) {
//         printf("Error ioctl\n");
//         printf ("Error no - %d\n", errno);
//         printf ("Error string - %s\n", strerror(errno));
//         exit(EXIT_FAILURE);
//     }

    read_bytes = write (fd, buffer, BUFFER_SIZE);
    //read_bytes = read (fd, buffer, BUFFER_SIZE);

    printf("%s\n", buffer);
    if (read_bytes < 0)
    {
        printf ("myread: Cannot read file\n");
        close (fd);
        close(fd1);
        exit (1);
    }
 
    exit (0);
}
