#include "aj-snapshot.h"

const char* xml_whitespace_cb( mxml_node_t *node, int where)
{
	const char *name;
	name = node->value.element.name;
	if ( !strcmp(name, "alsa") || !strcmp(name, "jack"))
	{
		if (where == MXML_WS_BEFORE_OPEN || where == MXML_WS_BEFORE_CLOSE)
		return ("\n");
	}
	if ( !strcmp(name, "client") )
	{
		if ( where == MXML_WS_BEFORE_OPEN )
		return ("\n  ");
	}
	return NULL;
}

void alsa_store_connected_clients( snd_seq_t* seq, mxml_node_t* alsa_node )
{
	const char* name;

	snd_seq_client_info_t* cinfo;
	snd_seq_client_info_alloca(&cinfo);
	snd_seq_client_info_set_client(cinfo, -1);

	while (snd_seq_query_next_client(seq, cinfo) >= 0) 
	{
		name = snd_seq_client_info_get_name(cinfo);

		mxml_node_t* client_node;
		client_node = mxmlNewElement(alsa_node, "client");
		mxmlElementSetAttr(client_node, "name", name);
	}
}

void alsa_store( snd_seq_t* seq, const char* filename )
{
	mxml_node_t* xml_node;
	mxml_node_t* alsa_node;

	xml_node = mxmlNewXML("1.0");
	alsa_node = mxmlNewElement(xml_node, "alsa");

	alsa_store_connected_clients( seq, alsa_node );

	FILE *fp;
	if( (fp = fopen(filename, "w")) == NULL ){
		perror("Could not open file");
		exit(1);
	}
	mxmlSaveFile(xml_node, fp, xml_whitespace_cb);
	fclose(fp);
}

snd_seq_t* alsa_initialize( snd_seq_t* seq )
{
        int client;

        if (snd_seq_open(&seq, "default", SND_SEQ_OPEN_DUPLEX, 0) < 0) {
                fprintf(stderr, "can't open sequencer\n");
                exit(1);
        }

        if ((client = snd_seq_client_id(seq)) < 0) {
                snd_seq_close(seq);
                fprintf(stderr, "can't get client id\n");
                exit(1);
        }

        if (snd_seq_set_client_name(seq, "aj-connect") < 0) {
                snd_seq_close(seq);
                fprintf(stderr, "can't set client info\n");
                exit(1);
        }
	return seq;
}
