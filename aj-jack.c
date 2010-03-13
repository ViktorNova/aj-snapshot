#include "aj-snapshot.h"

jack_client_t* jack_initialize( jack_client_t* jackc )
{
	jackc = jack_client_open("aj-connect", (jack_options_t)0, NULL);

	if (jackc == NULL) {
		fprintf(stderr, "Could not become jack client.");
		exit(1);
	}
	return jackc;
}

void jack_store_connections( jack_client_t* jackc, const char* port_name, mxml_node_t* port_node )
{

	mxml_node_t* connection_node;

	jack_port_t* port = jack_port_by_name(jackc, port_name);
	const char** connected_ports = jack_port_get_all_connections(jackc, port);

	if(connected_ports != NULL)
	{
		unsigned int i = 0;
		const char* connected_port = connected_ports[i];

		while (connected_port) 
		{
			connection_node = mxmlNewElement(port_node, "connection");
			mxmlElementSetAttr(connection_node, "port", connected_port);

			connected_port = connected_ports[++i];
		}
	}
}

void jack_store( jack_client_t* jackc, mxml_node_t* xml_node )
{
	mxml_node_t* jack_node = mxmlNewElement(xml_node, "jack");
	mxml_node_t* client_node;
	mxml_node_t* port_node;

	const char* sep = ":";
	char full_name[jack_port_name_size()];
	char client_name_prev[jack_port_name_size()];
	const char* client_name;
	char* port_name;

	const char **jack_output_ports = jack_get_ports(jackc, NULL, NULL, JackPortIsOutput);
	
	unsigned int i = 0;
	const char* full_name_const = jack_output_ports[i];

	while (full_name_const) 
	{
		strcpy(full_name, full_name_const); // otherwise we change the names of ports in jack !!

		client_name = strtok(full_name, sep);
		port_name = strtok(NULL, sep);

		if ( strcmp(client_name_prev, client_name) )
		{
			client_node = mxmlNewElement(jack_node, "client");
			mxmlElementSetAttr(client_node, "name", client_name);
			strcpy(client_name_prev, client_name);
		}

		port_node = mxmlNewElement(client_node, "port");
		mxmlElementSetAttr(port_node, "name", port_name);

		jack_store_connections(jackc, full_name_const, port_node);

		full_name_const = jack_output_ports[++i];
	}
}
