#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <linux/kd.h>

#include <SDL2/SDL.h>

const char *beepr_version_string = "0.0.1.4";

static const struct option long_options[] = {
	{"help", no_argument, NULL, 'h'},
	{"beep", required_argument, NULL, 'b'},
	{"daemon", no_argument, NULL, 'd'},
	{"dsp", no_argument, NULL, 'D'},
	{"frequency", required_argument, NULL, 'f'},
	{"ioctl", no_argument, NULL, 'i'},
	{"pipe", no_argument, NULL, 'p'},
	{"version", no_argument, NULL, 'V'},
	{"verbose", no_argument, NULL, 'v'},
	{NULL, 0, NULL, 0}
};
static const char *short_options = "hb:Ddf:ipVv";

#define BUFFER_SIZE 4096
char buffer[BUFFER_SIZE];

unsigned int freq = 440;
unsigned char beepr_channels = 1;
int beepr_frequency = 44100;
unsigned short beepr_format = AUDIO_U8;
unsigned short beepr_sample_size = 4096;
unsigned int verbose;

void beeprShowHelp(void) {
	printf("Usage: beepr [ OPTIONS ]\n"
"Options:\n"
"\t-h, --help		Show this help message\n"
"\t-b, --beep		Play a simple beep\n"
"\t-d, --daemon		Run in the background and listen to FIFO /abin/beepr-cmd\n"
"\t-D, --dsp		Write data on /dev/dsp\n"
"\t-f, --frequency	Set beep frequency in HZ\n"
"\t-i, --ioctl		Use ioctl() on /dev/tty0\n"
"\t-p, --pipe		Write to /abin/beepr-cmd\n"
"\t-V, --version	Show program version and exit\n"
"\t-v, --verbose	Show more information for debugging\n");
}

void beeprShowVersion(void) {
	printf("beepr %s\n", beepr_version_string);
}

void beeprMakeBuffer(unsigned int freq) {
	unsigned int wavelen = beepr_frequency / freq;
	if (verbose)
		printf("freq / wavelen: %d/%u\n", beepr_frequency, wavelen);
	int cnt, x, i;
	for (cnt = 0, x = 127; cnt < BUFFER_SIZE; cnt += wavelen) {
		if (cnt >= BUFFER_SIZE) break;
		// wave up
		for (i = 128; i <= 255; i++) {
			buffer[cnt] = x;
			x += 2;
		}
		// wave down
		for (i = 255; i >= 0; i--) {
			buffer[cnt] = x;
			x -= 2;
		}
		// wave up
		for (i = 0; i <= 127; i++) {
			buffer[cnt] = x;
			x += 2;
		}
	}
}

void beeprSDL_play(unsigned int freq) {
	beeprMakeBuffer(freq);
	SDL_PauseAudio(0);
	usleep(125000);
	SDL_PauseAudio(1);
}

void beeprSDL_callback(void *userdata, Uint8 *stream, int len) {
	snprintf((char *)stream, len, buffer);
}

void beeprSDL(void) {
	SDL_Init(SDL_INIT_AUDIO);

	SDL_AudioSpec desired, obtained;
	desired.freq = beepr_frequency;
	desired.format = beepr_format;
	desired.channels = beepr_channels;
	desired.samples = beepr_sample_size;
	desired.callback = beeprSDL_callback;
	SDL_OpenAudio(&desired, &obtained);

	memset(buffer,'#', BUFFER_SIZE);
	beeprSDL_play(440);
	beeprSDL_play(880);
	beeprSDL_play(512);
	beeprSDL_play(682);

	SDL_CloseAudio();
	SDL_Quit();
}

void beeprIoctl(void) {
	char *console_name = "/dev/console";
	if (verbose)
		printf("opening /dev/console\n");
	int console_fd = open(console_name, O_WRONLY);
	if (console_fd == -1) {
		fprintf(stderr, "beepr: Cannot open /dev/console\n");
		return;
	}

	if (verbose)
		printf("freq: %u\n", freq);
	ioctl(console_fd, KIOCSOUND, freq);
	usleep(100000);
	ioctl(console_fd, KIOCSOUND, 0);

	close(console_fd);
}

void beeprPipe(void) {
	FILE *fp = fopen("/abin/beepr-cmd", "w");
	if (fp == NULL) {
		fprintf(stderr, "beepr_pipe() error: Cannot open /abin/beepr-cmd: %s\n", strerror(errno));
		return;
	}

	char command[4];
	sprintf(command, "01\n");
	fwrite(command, 1, strlen(command), fp);

	fclose(fp);
}

void beeprPipeProcess(void) {
	char *tmpstr = NULL;
	size_t linesize, strsize = 0;
	while (1) {
		FILE *fp = fopen("/abin/beepr-cmd", "r");
		linesize = getline(&tmpstr, &strsize, fp);
		printf(":%s:%lu:%lu\n", tmpstr, strsize, linesize);
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
	fwrite(buffer, 1, 4096, fp);
	beeprMakeBuffer(2840);
	fwrite(buffer, 1, 4096, fp);
	beeprMakeBuffer(1640);
	fwrite(buffer, 1, 4096, fp);
	beeprMakeBuffer(440);
	fwrite(buffer, 1, 4096, fp);

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
		case 'b':
			beeprSDL();
			exit(0);
		case 'D':
			beeprDSP();
			exit(0);
		case 'd':
			beeprPipeProcess();
			exit (0);
		case 'f':
			freq = atoi(optarg);
			break;
		case 'i':
			beeprIoctl();
			exit(0);
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

	return 0;
}

