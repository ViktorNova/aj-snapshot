/************************************************************************/
/* Copyright (C) 2009 Lieven Moors                                      */
/*                                                                      */
/* This file is part of aj-snapshot.                                    */
/*                                                                      */
/* aj-snapshot is free software: you can redistribute it and/or modify  */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation, either version 3 of the License, or    */
/* (at your option) any later version.                                  */
/*                                                                      */
/* aj-snapshot is distributed in the hope that it will be useful,       */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                         */
/*                                                                      */
/* You should have received a copy of the GNU General Public License    */
/* along with Flarps.  If not, see <http://www.gnu.org/licenses/>.      */
/************************************************************************/

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

void jack_restore_connections( jack_client_t* jackc, const char* client_name, const char* port_name, mxml_node_t* port_node )
{
	mxml_node_t* connection_node;
	const char* dest_port;
	unsigned int s = strlen(client_name) + strlen(port_name) + 2;
	char src_port[s];

	snprintf(src_port, s, "%s:%s", client_name, port_name);

	connection_node = mxmlFindElement(port_node, port_node, "connection", NULL, NULL, MXML_DESCEND_FIRST);

	while (connection_node)
	{
		dest_port = mxmlElementGetAttr(connection_node, "port");

		int err = jack_connect(jackc, src_port, dest_port);

		if (err == 0) {
			fprintf(stdout, "Connecting port '%s' with '%s'\n", src_port, dest_port);
		}
		else if (err == EEXIST) {
			fprintf(stderr, "Port '%s' is already connected to '%s'\n", src_port, dest_port);
		}
		else {
			fprintf(stderr, "Connection from '%s' to '%s' failed!\n", src_port, dest_port);	
		}

		connection_node = mxmlFindElement(connection_node, port_node, "port", NULL, NULL, MXML_NO_DESCEND);
	}
}

void jack_restore_ports( jack_client_t* jackc, const char* client_name, mxml_node_t* client_node)
{
	mxml_node_t* port_node;
	const char* port_name;

	port_node = mxmlFindElement(client_node, client_node, "port", NULL, NULL, MXML_DESCEND_FIRST);

	while (port_node)
	{
		port_name = mxmlElementGetAttr(port_node, "name");

		jack_restore_connections(jackc, client_name, port_name, port_node);

		port_node = mxmlFindElement(port_node, client_node, "port", NULL, NULL, MXML_NO_DESCEND);
	}
}

void jack_restore_clients( jack_client_t* jackc, mxml_node_t* jack_node )
{
	mxml_node_t* client_node;
	const char* client_name;

	client_node = mxmlFindElement(jack_node, jack_node, "client", NULL, NULL, MXML_DESCEND_FIRST);

	while (client_node)
	{
		client_name = mxmlElementGetAttr(client_node, "name");

		jack_restore_ports(jackc, client_name, client_node);

		client_node = mxmlFindElement(client_node, jack_node, "client", NULL, NULL, MXML_NO_DESCEND);
	}
}

void jack_restore( jack_client_t* jackc, mxml_node_t* xml_node )
{
	mxml_node_t* jack_node;
	jack_node = mxmlFindElement(xml_node, xml_node, "jack", NULL, NULL, MXML_DESCEND_FIRST);
	jack_restore_clients(jackc, jack_node);
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
