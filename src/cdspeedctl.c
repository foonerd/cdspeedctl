// cdspeedctl.c - Universal CD-ROM speed control utility
// License: MIT or BSD-style
// Author: Your Name

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

#define VERSION "0.1"
#define DEFAULT_DEVICE "/dev/sr0"
#define DEFAULT_SG      NULL
#define RETRY_DELAY_SEC 1

static const char *short_opts = "d:s:g:r:qvh";
static const struct option long_opts[] = {
    {"device",  required_argument, 0, 'd'},
    {"speed",   required_argument, 0, 's'},
    {"sg",      required_argument, 0, 'g'},
    {"retry",   required_argument, 0, 'r'},
    {"quiet",   no_argument,       0, 'q'},
    {"verbose", no_argument,       0, 'v'},
    {"help",    no_argument,       0, 'h'},
    {0, 0, 0, 0}
};

void usage(const char *prog) {
    fprintf(stderr,
        "Usage: %s --device /dev/srX --speed N [--sg /dev/sgX] [--retry N] [--quiet|--verbose]\n"
        "Options:\n"
        "  -d, --device   CD-ROM device (default: /dev/sr0)\n"
        "  -s, --speed    Speed (e.g., 1, 2, 4)\n"
        "  -g, --sg       Optional SG device for fallback (e.g., /dev/sg1)\n"
        "  -r, --retry    Retry seconds if device not ready\n"
        "  -q, --quiet    Suppress output except fatal errors\n"
        "  -v, --verbose  Verbose debug output\n"
        "  -h, --help     Show this help\n", prog);
}

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

int main(int argc, char **argv) {
    const char *device = DEFAULT_DEVICE;
    const char *sg_device = DEFAULT_SG;
    int speed = 0;
    int retry_seconds = 0;
    bool quiet = false;
    bool verbose = false;

    int opt;
    while ((opt = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
        switch (opt) {
            case 'd': device = optarg; break;
            case 's': speed = atoi(optarg); break;
            case 'g': sg_device = optarg; break;
            case 'r': retry_seconds = atoi(optarg); break;
            case 'q': quiet = true; break;
            case 'v': verbose = true; break;
            case 'h': usage(argv[0]); return 64;
            default: usage(argv[0]); return 64;
        }
    }

    if (!speed || speed < 1) {
        if (!quiet) fprintf(stderr, "Error: --speed must be specified and > 0\n");
        return 64;
    }

    // Wait for device to be ready if requested
    for (int i = 0; i <= retry_seconds; i++) {
        int fd = open(device, O_RDONLY | O_NONBLOCK);
        if (fd >= 0) {
            close(fd);
            break;
        }
        if (i == retry_seconds) {
            if (!quiet) perror("Device not ready");
            return 1;
        }
        sleep(RETRY_DELAY_SEC);
    }

    // Try ioctl first
    if (set_speed_ioctl(device, speed, verbose) == 0) return 0;

    // Fallback to SG_IO
    if (sg_device) {
        if (set_speed_sgio(sg_device, speed, verbose) == 0) return 0;
    }

    if (!quiet) fprintf(stderr, "Failed to set speed on any interface\n");
    return 2;
}
