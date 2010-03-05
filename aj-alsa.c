#include "aj-snapshot.h"

const char* xml_whitespace_cb( mxml_node_t *node, int where)
{
	const char *name;
	name = node->value.element.name;
	if ( !strcmp(name, "alsa") || !strcmp(name, "jack") )
	{
		if (where == MXML_WS_BEFORE_OPEN || where == MXML_WS_BEFORE_CLOSE)
		return ("\n");
	}
	if ( !strcmp(name, "client") )
	{
		if ( where == MXML_WS_BEFORE_OPEN || where == MXML_WS_BEFORE_CLOSE)
		return ("\n  ");
	}
	if ( !strcmp(name, "port") )
	{
		if ( where == MXML_WS_BEFORE_OPEN || where == MXML_WS_BEFORE_CLOSE)
		return ("\n    ");
	}
	if ( !strcmp(name, "connection") )
	{
		if ( where == MXML_WS_BEFORE_OPEN || where == MXML_WS_BEFORE_CLOSE)
		return ("\n      ");
	}
	if ( !strcmp(name, "cid") || !strcmp(name, "pid") )
	{
		if ( where == MXML_WS_BEFORE_OPEN )
		return ("\n        ");
	}
	return NULL;
}

void alsa_store_connections( snd_seq_t* seq, const snd_seq_addr_t *addr, mxml_node_t* port_node )
{
	snd_seq_query_subscribe_t *subs;
	snd_seq_query_subscribe_alloca(&subs);
	snd_seq_query_subscribe_set_root(subs, addr);
	snd_seq_query_subscribe_set_type(subs, SND_SEQ_QUERY_SUBS_READ);
	snd_seq_query_subscribe_set_index(subs, 0);

	while (snd_seq_query_port_subscribers(seq, subs) >= 0)
	{
		const snd_seq_addr_t *addr;
		addr = snd_seq_query_subscribe_get_addr(subs);

		mxml_node_t* connection_node;
		connection_node = mxmlNewElement(port_node, "connection");

		mxml_node_t* client_node = mxmlNewElement(connection_node, "cid");
		mxmlNewInteger(client_node, (int)addr->client);
		mxml_node_t* port_node = mxmlNewElement(connection_node, "pid");
		mxmlNewInteger(port_node, (int)addr->port);

		snd_seq_query_subscribe_set_index(subs, snd_seq_query_subscribe_get_index(subs) + 1);
	}		
}

void alsa_store_ports( snd_seq_t* seq, snd_seq_client_info_t* cinfo, snd_seq_port_info_t* pinfo, mxml_node_t* client_node )
{
	const char* name;
	snd_seq_port_info_set_client(pinfo, snd_seq_client_info_get_client(cinfo));
	snd_seq_port_info_set_port(pinfo, -1);

	while (snd_seq_query_next_port(seq, pinfo) >= 0)
	{
		name = snd_seq_port_info_get_name( pinfo );
		mxml_node_t* port_node;
		port_node = mxmlNewElement(client_node, "port");
                mxmlElementSetAttr(port_node, "name", name);	

		alsa_store_connections(seq, snd_seq_port_info_get_addr(pinfo), port_node);
	}
}

void alsa_store_clients( snd_seq_t* seq, mxml_node_t* alsa_node )
{
	const char* name;

	snd_seq_client_info_t* cinfo;
	snd_seq_client_info_alloca(&cinfo);
	snd_seq_port_info_t *pinfo;
	snd_seq_port_info_alloca(&pinfo); // allocate once, and reuse for all clients

	snd_seq_client_info_set_client(cinfo, -1);

	while (snd_seq_query_next_client(seq, cinfo) >= 0) 
	{
		name = snd_seq_client_info_get_name(cinfo);

		mxml_node_t* client_node;
		client_node = mxmlNewElement(alsa_node, "client");
		mxmlElementSetAttr(client_node, "name", name);

		alsa_store_ports( seq, cinfo, pinfo, client_node );
	}
}

void alsa_store( snd_seq_t* seq, const char* filename )
{
	mxml_node_t* xml_node;
	mxml_node_t* alsa_node;

	xml_node = mxmlNewXML("1.0");
	alsa_node = mxmlNewElement(xml_node, "alsa");

	alsa_store_clients( seq, alsa_node );

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
