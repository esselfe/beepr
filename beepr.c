#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/kd.h>

#ifdef HAVE_SDL2
#include <SDL2/SDL.h>
#endif

const char *beepr_version_string = "0.1.8";

static const struct option long_options[] = {
	{"help", no_argument, NULL, 'h'},
#ifdef HAVE_SDL2
	{"beep", no_argument, NULL, 'b'},
	{"error", no_argument, NULL, 'e'},
#endif
	{"daemon", no_argument, NULL, 'd'},
	{"dsp", no_argument, NULL, 'D'},
	{"frequency", required_argument, NULL, 'f'},
	{"ioctl", no_argument, NULL, 'i'},
	{"length", required_argument, NULL, 'l'},
	{"pipe", no_argument, NULL, 'p'},
	{"version", no_argument, NULL, 'V'},
	{"verbose", no_argument, NULL, 'v'},
	{NULL, 0, NULL, 0}
};
#ifdef HAVE_SDL2
static const char *short_options = "hbDdef:il:pVv";
#else
static const char *short_options = "hDdf:il:pVv";
#endif

#define BEEPR_BUFFER_SIZE 4096
char beepr_buffer[BEEPR_BUFFER_SIZE];

unsigned int beepr_frequency = 440;
unsigned int beepr_length = 125;
unsigned int verbose;
unsigned int play_SDL_error;

void beeprShowHelp(void) {
	printf("Usage: beepr [ OPTIONS ]\n"
"Options:\n"
"\t-h, --help		Show this help message\n"
#ifdef HAVE_SDL2
"\t-b, --beep		Play a simple beep\n"
"\t-e, --error		Play a simple error beep\n"
#endif
"\t-d, --daemon		Run in the background and listen to FIFO /run/beepr-cmd\n"
"\t-D, --dsp		Write data on /dev/dsp\n"
"\t-f, --frequency	Set beep frequency in HZ\n"
"\t-i, --ioctl		Use ioctl() on /dev/console\n"
"\t-l, --length		Beep duration in milliseconds\n"
"\t-p, --pipe		Write to /run/beepr-cmd\n"
"\t-V, --version	Show program version and exit\n"
"\t-v, --verbose	Show more information for debugging\n");
}

void beeprShowVersion(void) {
	printf("beepr %s\n", beepr_version_string);
}

void beeprMakeBuffer(unsigned int freq) {
	unsigned int wavelen = 44100 / freq;
	if (verbose)
		printf("freq / wavelen: 44100/%u\n", wavelen);
	int cnt, x, i;
	for (cnt = 0, x = 127; cnt < BEEPR_BUFFER_SIZE; cnt += wavelen) {
		if (cnt >= BEEPR_BUFFER_SIZE) break;
		// wave up
		for (i = 128; i <= 255; i++) {
			beepr_buffer[cnt] = x;
			x += 2;
		}
		// wave down
		for (i = 255; i >= 0; i--) {
			beepr_buffer[cnt] = x;
			x -= 2;
		}
		// wave up
		for (i = 0; i <= 127; i++) {
			beepr_buffer[cnt] = x;
			x += 2;
		}
	}
}

#ifdef HAVE_SDL2
void beeprSDL_play(unsigned int freq) {
	beeprMakeBuffer(freq);
	SDL_PauseAudio(0);
	usleep(beepr_length*1000);
	SDL_PauseAudio(1);
}

void beeprSDL_callback(void *userdata, Uint8 *stream, int len) {
	snprintf((char *)stream, len, beepr_buffer);
}

void beeprSDL(void) {
	SDL_Init(SDL_INIT_AUDIO);

	SDL_AudioSpec desired, obtained;
	desired.freq = 44100;
	desired.format = AUDIO_U8;
	desired.channels = 1;
	desired.samples = 4096;
	desired.callback = beeprSDL_callback;
	SDL_OpenAudio(&desired, &obtained);

	memset(beepr_buffer, '#', 4096);
	if (play_SDL_error) {
		beeprSDL_play(1080);
		beeprSDL_play(882);
		beeprSDL_play(624);
		beeprSDL_play(440);
	}
	else {
		beeprSDL_play(440);
		beeprSDL_play(880);
		beeprSDL_play(1024);
		beeprSDL_play(1648);
	}

	SDL_CloseAudio();
	SDL_Quit();
}
#endif

void beeprIoctl(unsigned int freq) {
	char *console_name = "/dev/console";
	if (verbose)
		printf("opening /dev/console\n");
	int console_fd = open(console_name, O_WRONLY);
	if (console_fd == -1) {
		fprintf(stderr, "beepr: Cannot open /dev/console: %s\n", strerror(errno));
		return;
	}

	if (verbose)
		printf("ioctl on /dev/console @ %uHZ\n", freq);
	
	ioctl(console_fd, KIOCSOUND, 1193180/freq);
	usleep(beepr_length*1000);
	ioctl(console_fd, KIOCSOUND, 0);

	close(console_fd);
}

void beeprPipe(void) {
	FILE *fp = fopen("/run/beepr-cmd", "w");
	if (fp == NULL) {
		fprintf(stderr, "beeprPipe() error: Cannot open /run/beepr-cmd: %s\n", strerror(errno));
		return;
	}

	char command[5];
	sprintf(command, "440\n");
	fwrite(command, 1, strlen(command), fp);

	fclose(fp);
}

void beeprPipeDaemon(void) {
	if (getuid() != 0) {
		fprintf(stderr, "The beepr daemon should be run as the root user\n");
		exit(1);
	}
	char *tmpstr = (char *)malloc(4096);
	size_t strsize = 4096;
	FILE *fp = fopen("/run/beepr-cmd", "r");
	if (fp == NULL) {
		if (mkfifo("/run/beepr-cmd", 0666) == -1) {
			fprintf(stderr, "beeprPipeDaemon() error: Cannot create /run/beepr-cmd\n");
			exit(1);
		}
	}
	fclose(fp);
	while (1) {
		fp = fopen("/run/beepr-cmd", "r");
		getline(&tmpstr, &strsize, fp);
		beeprIoctl(atoi(tmpstr));
		memset(tmpstr, 0, 4096);
		strsize = 4096;
		fclose(fp);
	}
}

void beeprDSP(void) {
	FILE *fp = fopen("/dev/dsp", "w");
	if (fp == NULL) {
		fprintf(stderr, "beepr error: Cannot open /dev/dsp\n");
		exit(1);
	}

	beeprMakeBuffer(440);
	fwrite(beepr_buffer, 1, 4096, fp);
	beeprMakeBuffer(2840);
	fwrite(beepr_buffer, 1, 4096, fp);
	beeprMakeBuffer(1640);
	fwrite(beepr_buffer, 1, 4096, fp);
	beeprMakeBuffer(440);
	fwrite(beepr_buffer, 1, 4096, fp);

	fclose(fp);
}

int main(int argc, char **argv) {
	int c;
	while (1) {
		c = getopt_long(argc, argv, short_options, long_options, NULL);
		if (c == -1)
			break;
		switch (c) {
		case 'h':
			beeprShowHelp();
			exit(0);
#ifdef HAVE_SDL2
		case 'b':
			beeprSDL();
			exit(0);
		case 'e':
			play_SDL_error = 1;
			beeprSDL();
			exit(0);
#endif
		case 'D':
			beeprDSP();
			exit(0);
		case 'd':
			beeprPipeDaemon();
			exit (0);
		case 'f':
			beepr_frequency = atoi(optarg);
			break;
		case 'i':
			beeprIoctl(beepr_frequency);
			exit(0);
		case 'l':
			beepr_length = atoi(optarg);
			break;
		case 'p':
			beeprPipe();
			exit(0);
		case 'V':
			beeprShowVersion();
			exit(0);
		case 'v':
			verbose = 1;
			break;
		}
	}
#ifdef HAVE_SDL2
	beeprSDL();
#else
	beeprIoctl(beepr_frequency);
#endif

	return 0;
}

