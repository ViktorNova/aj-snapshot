#ifndef AJ_ALSA_H
#define AJ_ALSA_H

snd_seq_t* alsa_initialize( snd_seq_t* seq );
void alsa_store( snd_seq_t* seq, const char* filename );
void alsa_store_clients( snd_seq_t* seq, mxml_node_t* alsa_node );
void alsa_store_ports( snd_seq_t* seq, snd_seq_client_info_t* cinfo, snd_seq_port_info_t* pinfo, mxml_node_t* client_node );
void alsa_store_connections( snd_seq_t* seq, const snd_seq_addr_t* addr, mxml_node_t* port_node );
void alsa_restore( snd_seq_t* seq, const char* filename );
void alsa_restore_clients( snd_seq_t* seq, mxml_node_t* alsa_node );
void alsa_restore_ports( snd_seq_t* seq, const char* client_name, mxml_node_t* client_node);
void alsa_restore_connections( snd_seq_t* seq, const char* client_name, int port_id, mxml_node_t* port_node);


#endif
