#include "aj-snapshot.h"

void alsa_remove_connection(snd_seq_t* seq, snd_seq_port_info_t* pinfo)
{
	snd_seq_query_subscribe_t *query;
	snd_seq_query_subscribe_alloca(&query);
	snd_seq_query_subscribe_set_root(query, snd_seq_port_info_get_addr(pinfo));
	snd_seq_query_subscribe_set_type(query, SND_SEQ_QUERY_SUBS_READ);

	int i = 0;
	snd_seq_query_subscribe_set_index(query, i);

	while(snd_seq_query_port_subscribers(seq, query) == 0)
	{
		snd_seq_port_info_t *port;
		snd_seq_port_info_alloca(&port);

		const snd_seq_addr_t *sender = snd_seq_query_subscribe_get_root(query);
		const snd_seq_addr_t *dest = snd_seq_query_subscribe_get_addr(query);

		printf("sender = %d:%d\tdestination = %d:%d\n", sender->client, sender->port, dest->client, dest->port);

		snd_seq_port_subscribe_t *subs;
		snd_seq_port_subscribe_alloca(&subs);
		snd_seq_port_subscribe_set_sender(subs, sender);
		snd_seq_port_subscribe_set_dest(subs, dest);

		if(snd_seq_unsubscribe_port(seq, subs) < 0){
			printf("could not unsubscribe connection\n\n");
			snd_seq_query_subscribe_set_index(query, ++i);
		}
		else {
			printf("unsubscribed connection!\n\n"); //don't update index!
		}
	}
}

void a_remove_connection(snd_seq_t* seq, snd_seq_port_info_t* pinfo)
{
	snd_seq_query_subscribe_t *query;
	snd_seq_query_subscribe_alloca(&query);
	snd_seq_query_subscribe_set_root(query, snd_seq_port_info_get_addr(pinfo));
	snd_seq_query_subscribe_set_type(query, SND_SEQ_QUERY_SUBS_READ);
	snd_seq_query_subscribe_set_index(query, 0);

	int i;
	snd_seq_query_port_subscribers(seq, query);
	int n = snd_seq_query_subscribe_get_num_subs(query);
	printf("port '%s' has got %d read subscriptions\n", snd_seq_port_info_get_name(pinfo), n);
		
	for(i = 0; i < n; ++i)
	{
		snd_seq_query_subscribe_set_index(query, i);
		snd_seq_query_port_subscribers(seq, query);

		snd_seq_port_info_t *port;
		snd_seq_port_info_alloca(&port);

		const snd_seq_addr_t *sender = snd_seq_query_subscribe_get_root(query);
		const snd_seq_addr_t *dest = snd_seq_query_subscribe_get_addr(query);

		printf("sender = %d:%d\tdestination = %d:%d\n", sender->client, sender->port, dest->client, dest->port);

		snd_seq_port_subscribe_t *subs;
		snd_seq_port_subscribe_alloca(&subs);
		snd_seq_port_subscribe_set_sender(subs, sender);
		snd_seq_port_subscribe_set_dest(subs, dest);

		if(snd_seq_unsubscribe_port(seq, subs) < 0)
			printf("could not unsubscribe connection\n\n");
		else printf("unsubscribed connection!\n\n");
	}	
}

void sa_remove_connection(snd_seq_t* seq, snd_seq_port_info_t* pinfo)
{
	snd_seq_query_subscribe_t *query;
	snd_seq_query_subscribe_alloca(&query);
	snd_seq_query_subscribe_set_root(query, snd_seq_port_info_get_addr(pinfo));
	snd_seq_query_subscribe_set_type(query, SND_SEQ_QUERY_SUBS_READ);
	snd_seq_query_subscribe_set_index(query, 0);

	int i = 7;
	for (; (snd_seq_query_port_subscribers(seq, query) >= 0) && (i > 0);
		snd_seq_query_subscribe_set_index(query, snd_seq_query_subscribe_get_index(query) + 1), --i)
	{
		int n = snd_seq_query_subscribe_get_num_subs(query);
		printf("port '%s' has got %d read subscriptions\n", snd_seq_port_info_get_name(pinfo), n);

		snd_seq_port_info_t *port;
		snd_seq_port_info_alloca(&port);

		const snd_seq_addr_t *sender = snd_seq_query_subscribe_get_root(query);
		const snd_seq_addr_t *dest = snd_seq_query_subscribe_get_addr(query);

		printf("sender = %d:%d\tdestination = %d:%d\n", sender->client, sender->port, dest->client, dest->port);

		if (snd_seq_get_any_port_info(seq, dest->client, dest->port, port) < 0){
			printf("could not get port info for destination port '%d:%d'\n", dest->client, dest->port);
			continue;
		}
		snd_seq_port_subscribe_t *subs;
		snd_seq_port_subscribe_alloca(&subs);
		snd_seq_port_subscribe_set_queue(subs, snd_seq_query_subscribe_get_queue(query));
		snd_seq_port_subscribe_set_sender(subs, sender);
		snd_seq_port_subscribe_set_dest(subs, dest);
		if(snd_seq_unsubscribe_port(seq, subs) < 0)
			printf("could not unsubscribe connection\n\n");
		else printf("unsubscribed connection!\n\n");
	}
}

void lsa_remove_connection(snd_seq_t *seq, snd_seq_port_info_t *pinfo)
{
	snd_seq_query_subscribe_t *query;

	snd_seq_query_subscribe_alloca(&query);
	snd_seq_query_subscribe_set_root(query, snd_seq_port_info_get_addr(pinfo));
	printf("QUERY FOR SOURCE PORT: '%s'\n", snd_seq_port_info_get_name(pinfo));

	snd_seq_query_subscribe_set_type(query, SND_SEQ_QUERY_SUBS_READ);
	snd_seq_query_subscribe_set_index(query, 0);

	for (; snd_seq_query_port_subscribers(seq, query) >= 0;
		snd_seq_query_subscribe_set_index(query, snd_seq_query_subscribe_get_index(query) + 1))
	{
		printf("subscribe_index = %d\n", snd_seq_query_subscribe_get_index(query));
		snd_seq_port_info_t *port;
		snd_seq_port_subscribe_t *subs;

		const snd_seq_addr_t *sender = snd_seq_query_subscribe_get_root(query);
		const snd_seq_addr_t *dest = snd_seq_query_subscribe_get_addr(query);

		snd_seq_port_info_alloca(&port);

		if (snd_seq_get_any_port_info(seq, dest->client, dest->port, port) < 0){
			printf("could not get port info for destination port '%d:%d'\n", dest->client, dest->port);
			continue;
		}
		if (!(snd_seq_port_info_get_capability(port) & SND_SEQ_PORT_CAP_SUBS_WRITE)){
			printf("destination port '%s' has no write subscription capabily\n", snd_seq_port_info_get_name(port));
			continue;
		}
		if (snd_seq_port_info_get_capability(port) & SND_SEQ_PORT_CAP_NO_EXPORT){
			printf("destination port '%s' is set to NO_EXPORT\n", snd_seq_port_info_get_name(port));
			continue;
		}
		printf("connection from '%s' ", snd_seq_port_info_get_name(pinfo));
		printf("to '%s' is gonna be removed\n", snd_seq_port_info_get_name(port));

		snd_seq_port_subscribe_alloca(&subs);
		snd_seq_port_subscribe_set_queue(subs, snd_seq_query_subscribe_get_queue(query));
		snd_seq_port_subscribe_set_sender(subs, sender);
		snd_seq_port_subscribe_set_dest(subs, dest);
		snd_seq_unsubscribe_port(seq, subs);
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
	/* reset query info */
		snd_seq_port_info_set_client(pinfo, snd_seq_client_info_get_client(cinfo));
		snd_seq_port_info_set_port(pinfo, -1);

		while (snd_seq_query_next_port(seq, pinfo) >= 0) 
		{
			alsa_remove_connection(seq, pinfo);
		}
	}
}
