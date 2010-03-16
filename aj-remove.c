#include "aj-snapshot.h"

void alsa_remove_connection(snd_seq_t* seq, snd_seq_port_info_t* pinfo)
{
	snd_seq_query_subscribe_t *query;
	snd_seq_query_subscribe_alloca(&query);
	snd_seq_query_subscribe_set_root(query, snd_seq_port_info_get_addr(pinfo));
	snd_seq_query_subscribe_set_type(query, SND_SEQ_QUERY_SUBS_READ);
	snd_seq_query_subscribe_set_index(query, 0);

	while(snd_seq_query_port_subscribers(seq, query) == 0)
	{
		snd_seq_port_info_t *port;
		snd_seq_port_info_alloca(&port);

		const snd_seq_addr_t *sender = snd_seq_query_subscribe_get_root(query);
		const snd_seq_addr_t *dest = snd_seq_query_subscribe_get_addr(query);

		if ((snd_seq_get_any_port_info(seq, dest->client, dest->port, port) < 0) ||
			!(snd_seq_port_info_get_capability(port) & SND_SEQ_PORT_CAP_SUBS_WRITE) ||
			(snd_seq_port_info_get_capability(port) & SND_SEQ_PORT_CAP_NO_EXPORT))
		{
			snd_seq_query_subscribe_set_index(query, snd_seq_query_subscribe_get_index(query) + 1);
			continue;
		}

		snd_seq_port_subscribe_t *subs;
		snd_seq_port_subscribe_alloca(&subs);
		snd_seq_port_subscribe_set_queue(subs, snd_seq_query_subscribe_get_queue(query));
		snd_seq_port_subscribe_set_sender(subs, sender);
		snd_seq_port_subscribe_set_dest(subs, dest);

		if(snd_seq_unsubscribe_port(seq, subs) < 0){
			snd_seq_query_subscribe_set_index(query, snd_seq_query_subscribe_get_index(query) + 1);
		}
	}
}

void alsa_remove_connections(snd_seq_t *seq)
{
        snd_seq_client_info_t *cinfo;
        snd_seq_client_info_alloca(&cinfo);
        snd_seq_port_info_t *pinfo;
        snd_seq_port_info_alloca(&pinfo);

        snd_seq_client_info_set_client(cinfo, -1);

        while (snd_seq_query_next_client(seq, cinfo) >= 0) {
                snd_seq_port_info_set_client(pinfo, snd_seq_client_info_get_client(cinfo));
                snd_seq_port_info_set_port(pinfo, -1);

                while (snd_seq_query_next_port(seq, pinfo) >= 0)
                {
                        alsa_remove_connection(seq, pinfo);
                }
        }
}

