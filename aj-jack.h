#ifndef AJ_JACK_H
#define AJ_JACK_H

jack_client_t* jack_initialize( jack_client_t* jackc );
void jack_store( jack_client_t* jackc, mxml_node_t* xml_node );

#endif
