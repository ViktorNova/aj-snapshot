#ifndef AJ_FILE_H
#define AJ_FILE_H

const char* xml_whitespace_cb( mxml_node_t *node, int where );
mxml_node_t* read_xml( const char* filename, mxml_node_t* xml );
void write_xml( const char* filename, mxml_node_t* xml );

#endif
