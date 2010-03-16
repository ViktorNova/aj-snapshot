#ifndef AJ_REMOVE_H
#define AJ_REMOVE_H

void alsa_remove_connections(snd_seq_t *seq);
void jack_remove_connections(jack_client_t* jackc);

#endif
