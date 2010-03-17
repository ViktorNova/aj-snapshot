#ifndef AJ_ALSA_H
#define AJ_ALSA_H

snd_seq_t* alsa_initialize( snd_seq_t* seq );
void alsa_store( snd_seq_t* seq, mxml_node_t* xml_node );
void alsa_restore( snd_seq_t* seq, mxml_node_t* xml_node );


#endif
