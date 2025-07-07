/* test_sr501.c */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#define DEVICE_PATH "/dev/sr501"

int main(void) {
    int fd;
    int ret;
    int read_val;

    // 1. Open the device file
    fd = open(DEVICE_PATH, O_RDONLY);
    if (fd < 0) {
        // perror will print an error message based on errno, e.g., "No such file or directory"
        perror("Failed to open device " DEVICE_PATH);
        return -1;
    }

    printf("Device %s opened successfully. Waiting for motion...\n", DEVICE_PATH);

    // 2. Enter an infinite loop to listen continuously
    while (1) {
        printf("Blocking on read()... Trigger the sensor.\n");

        // 3. Read from the device, this will block until an interrupt occurs
        // The driver will return an integer (4 bytes), so we prepare an int to receive it
        ret = read(fd, &read_val, sizeof(read_val));

        if (ret > 0) {
            // 4. read() returns, indicating an event was detected
            // In our driver, read_val should be 1
            printf("Event detected! read() returned %d bytes. Value = %d\n\n", ret, read_val);
        } else if (ret == 0) {
            // This typically doesn't happen with this kind of driver
            fprintf(stderr, "End of file reached.\n");
            break;
        } else { // ret < 0
            // If read is interrupted by a signal (e.g., Ctrl+C), errno will be EINTR
            if (errno == EINTR) {
                printf("\nRead interrupted by a signal. Exiting.\n");
            } else {
                perror("Read error");
            }
            break;
        }
    }

    // 5. Close the file descriptor
    close(fd);
    printf("Device closed.\n");

    return 0;
}