#include "aj-snapshot.h"
#include "aj-alsa.h"
#include "aj-file.h"
#include "aj-jack.h"

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
	mxml_node_t* xml_node = NULL;
	snd_seq_t* seq = NULL;
	jack_client_t* jackc = NULL;

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
					xml_node = mxmlNewXML("1.0");
					alsa_store(seq, xml_node);
					write_xml(filename, xml_node);
					break;
				case RESTORE:
					printf("ALSA RESTORE\n");
					xml_node = read_xml(filename, xml_node);
					alsa_restore(seq, xml_node);
					break;
			}
			snd_seq_close(seq);
			break;
		case JACK:
			jackc = jack_initialize(jackc);
			switch (action){
				case STORE:
					printf("JACK STORE\n");
					xml_node = mxmlNewXML("1.0");
					jack_store(jackc, xml_node);
					write_xml(filename, xml_node);
					break;
				case RESTORE:
					printf("JACK RESTORE\n");
					break;
			}
			break;
			jack_client_close(jackc);
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
	return 0;
}
