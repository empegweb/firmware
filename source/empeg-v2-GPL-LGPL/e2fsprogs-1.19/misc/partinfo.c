/*
 * partinfo.c
 *
 * Originally written by Alain Knaff, <alknaff@innet.lu>.
 *
 * Cleaned up by Theodore Ts'o, <tytso@mit.edu>.
 * 
 */

#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <linux/hdreg.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include "nls-enable.h"

void print_error(char *operation, int error, char *device)
{
	fprintf(stderr, _("%s failed for %s: %s\n"), operation, device,
		strerror(error));
}

int main(int argc, char **argv)
{
	struct hd_geometry loc;
	int fd, i;
	long size;

#ifdef ENABLE_NLS
	setlocale(LC_MESSAGES, "");
	bindtextdomain(NLS_CAT_NAME, LOCALEDIR);
	textdomain(NLS_CAT_NAME);
#endif
	if (argc == 1) {
		fprintf(stderr, _("Usage: %s <dev1> <dev2> <dev3>\n\n"
			"This program prints out the partition information "
			"for a set of devices\n"
			"A common way to use this program is:\n\n\t"
			"%s /dev/hda?\n\n"), argv[0], argv[0]);
		exit(1);
	}
    
	for (i=1; i < argc; i++) {
		fd = open(argv[i], O_RDONLY);

		if (fd < 0) {
			print_error("open", errno, argv[i]);
			continue;
		}
    
		if (ioctl(fd, HDIO_GETGEO, &loc) < 0) {
			print_error(_("HDIO_GETGEO ioctl"), errno, argv[i]);
			close(fd);
			continue;
		}
    
    
		if (ioctl(fd, BLKGETSIZE, &size) < 0) {
			print_error(_("BLKGETSIZE ioctl"), errno, argv[i]);
			close(fd);
			continue;
		}
    
		printf("%s: h=%3d s=%3d c=%4d   start=%8d size=%8d end=%8d\n",
		       argv[i], 
		       loc.heads, (int)loc.sectors, loc.cylinders,
		       (int)loc.start, size, (int) loc.start + size -1);
		close(fd);
	}
	exit(0);
}
