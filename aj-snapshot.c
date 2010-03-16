#include "aj-snapshot.h"
#include "aj-alsa.h"
#include "aj-file.h"
#include "aj-jack.h"
#include "aj-remove.h"

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
	{"remove-connections", 0, NULL, 'x'},
};

int main(int argc, char **argv)
{ 
	int c;
	int remove_connections = 0;
	enum sys system = ALSA_JACK;
	enum act action = STORE;
	static const char *filename;
	mxml_node_t* xml_node = NULL;
	snd_seq_t* seq = NULL;
	jack_client_t* jackc = NULL;

	while ((c = getopt_long(argc, argv, "ajrx", long_option, NULL)) != -1) {

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
		case 'x':
			remove_connections = 1;
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
			switch (action){
				case STORE:
					if(remove_connections) alsa_remove_connections(seq);
					xml_node = mxmlNewXML("1.0");
					alsa_store(seq, xml_node);
					write_xml(filename, xml_node);
					fprintf(stdout, "ALSA connections stored!\n");
					break;
				case RESTORE:
					xml_node = read_xml(filename, xml_node);
					alsa_restore(seq, xml_node);
					fprintf(stdout, "ALSA connections restored!\n");
					break;
			}
			snd_seq_close(seq);
			break;
		case JACK:
			jackc = jack_initialize(jackc);
			switch (action){
				case STORE:
					xml_node = mxmlNewXML("1.0");
					jack_store(jackc, xml_node);
					write_xml(filename, xml_node);
					mxmlDelete(xml_node);
					fprintf(stdout, "JACK connections stored!\n");
					break;
				case RESTORE:
					xml_node = read_xml(filename, xml_node);
					jack_restore(jackc, xml_node);
					mxmlDelete(xml_node);
					fprintf(stdout, "JACK connections restored!\n");
					break;
			}
			break;
			jack_client_close(jackc);
		case ALSA_JACK:
			seq = alsa_initialize(seq);
			jackc = jack_initialize(jackc);
			switch (action){
				case STORE:
					xml_node = mxmlNewXML("1.0");
					alsa_store(seq, xml_node);
					jack_store(jackc, xml_node);
					write_xml(filename, xml_node);
					mxmlDelete(xml_node);
					fprintf(stdout, "ALSA & JACK connections stored!\n");
					break;
				case RESTORE:
					xml_node = read_xml(filename, xml_node);
					alsa_restore(seq, xml_node);
					jack_restore(jackc, xml_node);
					mxmlDelete(xml_node);
					fprintf(stdout, "ALSA & JACK connections restored!\n");
					break;
			}
			snd_seq_close(seq);
			jack_client_close(jackc);
			break;
	}
	return 0;
}
