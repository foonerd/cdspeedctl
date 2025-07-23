// cdspeedctl.c - Universal CD-ROM speed control utility
// License: MIT or BSD-style
// Author: Andrew Seredyn

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <linux/cdrom.h>
#include <sys/ioctl.h>
#include <scsi/sg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>

#ifndef CDROM_GET_SPEED
#define CDROM_GET_SPEED 0x5323
#endif

#define VERSION "0.2"
#define DEFAULT_DEVICE "/dev/sr0"
#define DEFAULT_SG      NULL
#define RETRY_DELAY_SEC 1

static const char *short_opts = "d:s:g:r:qvhc";
static const struct option long_opts[] = {
    {"device",  required_argument, 0, 'd'},
    {"speed",   required_argument, 0, 's'},
    {"sg",      required_argument, 0, 'g'},
    {"retry",   required_argument, 0, 'r'},
    {"quiet",   no_argument,       0, 'q'},
    {"verbose", no_argument,       0, 'v'},
    {"current", no_argument,       0, 'c'},
    {"help",    no_argument,       0, 'h'},
    {0, 0, 0, 0}
};

// Usage message
void usage(const char *prog) {
    fprintf(stderr,
        "Usage: %s --device /dev/srX --speed N [--sg /dev/sgX] [--retry N] [--quiet|--verbose] [-c]\n"
        "Options:\n"
        "  -d, --device   CD-ROM device (default: /dev/sr0)\n"
        "  -s, --speed    Speed (e.g., 1, 2, 4)\n"
        "  -g, --sg       Optional SG device for fallback (e.g., /dev/sg1)\n"
        "  -r, --retry    Retry seconds if device not ready\n"
        "  -q, --quiet    Suppress output except fatal errors\n"
        "  -v, --verbose  Verbose debug output\n"
        "  -c, --current  Get the current speed of the CD-ROM drive\n"
        "  -h, --help     Show this help\n", prog);
}

// Function to get the current speed using ioctl (CDROM_GET_SPEED)
int get_speed_ioctl(const char *device, bool verbose) {
    int fd = open(device, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        if (verbose) perror("open ioctl device");
        return -1;
    }

    int speed;
    if (ioctl(fd, CDROM_GET_SPEED, &speed) == 0) {
        if (verbose) printf("Current speed: %d\n", speed);
        close(fd);
        return speed;
    }

    if (verbose) perror("CDROM_GET_SPEED failed");
    close(fd);
    return -1;
}

// Function to set speed using ioctl (CDROM_SELECT_SPEED)
int set_speed_ioctl(const char *device, int speed, bool verbose) {
    int fd = open(device, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        if (verbose) perror("open ioctl device");
        return -1;
    }

    if (verbose) printf("Trying ioctl CDROM_SELECT_SPEED on %s...\n", device);
    int result = ioctl(fd, CDROM_SELECT_SPEED, speed);
    close(fd);

    if (result == 0) {
        if (verbose) printf("Speed set via ioctl successfully.\n");
        return 0;
    }

    if (verbose) perror("CDROM_SELECT_SPEED failed");
    return -1;
}

// Function to set speed using SG_IO (SCSI interface)
int set_speed_sgio(const char *sg_device, int speed, bool verbose) {
    int fd = open(sg_device, O_RDWR);
    if (fd < 0) {
        if (verbose) perror("open SG device");
        return -1;
    }

    if (verbose) printf("Trying SG_IO SCSI SET CD SPEED on %s...\n", sg_device);

    unsigned char cdb[12] = {
        0xBB, 0x00,
        (speed >> 8) & 0xFF, speed & 0xFF,   // Read speed
        0x00, 0x00,                          // Write speed
        0x00, 0x00, 0x00, 0x00, 0x00
    };

    unsigned char sense[32] = {0};
    sg_io_hdr_t io_hdr = {
        .interface_id    = 'S',
        .dxfer_direction = SG_DXFER_NONE,
        .cmd_len         = sizeof(cdb),
        .mx_sb_len       = sizeof(sense),
        .cmdp            = cdb,
        .sbp             = sense,
        .timeout         = 5000
    };

    int result = ioctl(fd, SG_IO, &io_hdr);
    close(fd);

    if (result < 0) {
        if (verbose) perror("SG_IO ioctl");
        return -1;
    }

    if ((io_hdr.info & SG_INFO_OK_MASK) != SG_INFO_OK) {
        if (verbose) fprintf(stderr, "SG_IO failed: sense key = 0x%x\n", sense[2]);
        return -1;
    }

    if (verbose) printf("Speed set via SG_IO successfully.\n");
    return 0;
}

// Retry logic for setting speed
int retry_set_speed(const char *device, const char *sg_device, int speed, int retries, bool verbose) {
    int attempts = 0;
    int result = -1;

    while (attempts < retries) {
        if (verbose) {
            printf("[+] Attempt %d to set speed to %d...\n", attempts + 1, speed);
        }

        // Try setting speed with ioctl first
        result = set_speed_ioctl(device, speed, verbose);
        if (result == 0) {
            if (verbose) {
                printf("[+] Speed set successfully on attempt %d\n", attempts + 1);
            }
            break;
        }

        // If ioctl fails, fallback to SG_IO
        if (result != 0 && sg_device) {
            if (set_speed_sgio(sg_device, speed, verbose) == 0) {
                if (verbose) {
                    printf("[+] Speed set successfully using SG_IO on attempt %d\n", attempts + 1);
                }
                result = 0;
                break;
            }
        }

        attempts++;
        sleep(RETRY_DELAY_SEC);  // wait before retrying
    }

    // After retries, check if it was successful
    if (result == 0) {
        get_speed_ioctl(device, verbose);  // report current speed after success
    } else {
        if (verbose) {
            fprintf(stderr, "[+] Failed to set speed after %d attempts\n", retries);
        }
    }

    return result;
}

// Main function
int main(int argc, char **argv) {
    const char *device = DEFAULT_DEVICE;
    const char *sg_device = DEFAULT_SG;
    int speed = 0;
    int retry_seconds = 0;
    bool quiet = false;
    bool verbose = false;
    bool get_speed = false;

    int opt;
    while ((opt = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
        switch (opt) {
            case 'd': device = optarg; break;
            case 's': speed = atoi(optarg); break;
            case 'g': sg_device = optarg; break;
            case 'r': retry_seconds = atoi(optarg); break;
            case 'q': quiet = true; break;
            case 'v': verbose = true; break;
            case 'c': get_speed = true; break;  // current speed flag
            case 'h': usage(argv[0]); return 64;
            default: usage(argv[0]); return 64;
        }
    }

    // Check for the -c flag (current speed request)
    if (get_speed) {
        if (get_speed_ioctl(device, verbose) < 0) {
            return 1;
        }
        return 0;
    }

    // Validate speed
    if (!speed || speed < 1) {
        if (!quiet) fprintf(stderr, "Error: --speed must be specified and > 0\n");
        return 64;
    }

    // Wait for device to be ready if requested
    for (int i = 0; i <= retry_seconds; i++) {
        int fd = open(device, O_RDONLY | O_NONBLOCK);
        if (fd >= 0) {
            close(fd); // Device is ready, proceed
            break;
        }
        if (i == retry_seconds) {
            if (!quiet) perror("Device not ready");
            return 1;
        }
        sleep(RETRY_DELAY_SEC);
    }

    // Try setting speed with retries
    if (retry_set_speed(device, sg_device, speed, retry_seconds ? retry_seconds : 3, verbose) == 0) {
        return 0;
    }

    return 2;  // Failure after retries
}
