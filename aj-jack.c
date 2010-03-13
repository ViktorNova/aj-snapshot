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

void jack_store( jack_client_t* jackc, mxml_node_t* xml_node )
{
	mxml_node_t* jack_node = mxmlNewElement(xml_node, "jack");

	mxml_node_t* client_node;
	mxml_node_t* port_node;

	const char **jack_output_ports = jack_get_ports(jackc, NULL, NULL, JackPortIsOutput);

	char client_name_prev[jack_port_name_size()];
	const char* client_name;
	char* port_name;
	const char* sep = ":";
	
	unsigned int i = 0;
	const char* full_name_const = jack_output_ports[i];
	char full_name[jack_port_name_size()];

	while (full_name_const) 
	{
		strcpy(full_name, full_name_const); // otherwise we change the names of ports in jack !!

		client_name = strtok(full_name, sep);
		port_name = strtok(NULL, sep);
		printf("port_name = %s\n", port_name);

		++i;
		full_name_const = jack_output_ports[i];

		if ( !strcmp(client_name_prev, client_name) ){
			port_node = mxmlNewElement(client_node, "port");
			mxmlElementSetAttr(port_node, "name", port_name);
			continue;
		}
		else strcpy(client_name_prev, client_name);

		client_node = mxmlNewElement(jack_node, "client");
		mxmlElementSetAttr(client_node, "name", client_name);

		port_node = mxmlNewElement(client_node, "port");
		mxmlElementSetAttr(port_node, "name", port_name);

		//jack_store_connections();
	}
}
