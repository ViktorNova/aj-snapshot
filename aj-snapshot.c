#include "aj-snapshot.h"
#include "aj-alsa.h"

static void usage(void)
{
	printf("aj-snapshot\n");
} 

enum sys {
	ALSA, JACK, ALSA_JACK
};

enum act {
	STORE, RESTORE
};

static const struct option long_option[] = {
	{"alsa", 0, NULL, 'a'},
	{"jack", 0, NULL, 'j'},
	{"restore", 0, NULL, 'r'},
	{"file", 1, NULL, 'f'},
};

int main(int argc, char **argv)
{ 
	int c;
	enum sys system = ALSA_JACK;
	enum act action = STORE;
	static const char *filename;
	snd_seq_t* seq = NULL;

	while ((c = getopt_long(argc, argv, "ajrf:", long_option, NULL)) != -1) {

		switch (c){

		case 'a':
			system = ALSA;
			break;
		case 'j':
			system = JACK;
			break;
		case 'r':
			action = RESTORE;
			break;
		default:
			usage();
			return 1;
		}
	}

	if (argc == (optind + 1)) {
		filename = argv[optind];
	}
	else {
		fprintf(stderr, "Please specify one file to store/restore the snapshot\n");
		return 1;
	}

	

	switch (system) {
		case ALSA:
			seq = alsa_initialize(seq);
			//printf("id = %d", snd_seq_client_id(seq));
			switch (action){
				case STORE:
				printf("ALSA STORE\n");
				alsa_store(seq, filename);
				break;
				case RESTORE:
				printf("ALSA RESTORE\n");
				alsa_restore(seq, filename);
				break;
			}
			break;
		case JACK:
			switch (action){
				case STORE:
				printf("JACK STORE\n");
				break;
				case RESTORE:
				printf("JACK RESTORE\n");
				break;
			}
			break;
		case ALSA_JACK:
			switch (action){
				case STORE:
				printf("ALSA STORE\n");
				printf("JACK STORE\n");
				break;
				case RESTORE:
				printf("ALSA RESTORE\n");
				printf("JACK RESTORE\n");
				break;
			}
			break;
	}
	//snd_seq_close(seq);
	return 0;
}
