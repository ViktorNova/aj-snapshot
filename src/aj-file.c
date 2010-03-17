#include "aj-snapshot.h"

const char* xml_whitespace_cb( mxml_node_t *node, int where )
{
	const char *name;
	name = node->value.element.name;

	if (where == MXML_WS_BEFORE_OPEN || where == MXML_WS_BEFORE_CLOSE)
	{
		if ( !strcmp(name, "alsa") || !strcmp(name, "jack") ){
		return ("\n");
		}
		if ( !strcmp(name, "client") ){
		return ("\n  ");
		}
		if ( !strcmp(name, "port") ){
		return ("\n    ");
		}
		if ( !strcmp(name, "connection") ){
		return ("\n      ");
		}
	}
	return NULL;
}


mxml_node_t* read_xml( const char* filename, mxml_node_t* xml_node )
{
	FILE* file;

	if( (file = fopen(filename, "r")) == NULL ){
		perror("Could not open file for reading");
		exit(1);
	}

	xml_node = mxmlLoadFile(NULL, file, MXML_TEXT_CALLBACK);
	fclose(file);
	
	return xml_node;
}

void write_xml( const char* filename, mxml_node_t* xml_node )
{
	FILE* file;

	if( (file = fopen(filename, "w")) == NULL ){
		perror("Could not open file for writing");
		exit(1);
	}
	mxmlSaveFile(xml_node, file, xml_whitespace_cb);
	fclose(file);
}
