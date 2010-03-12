#include "aj-snapshot.h"

jack_client_t* jack_initialize( jack_client_t* jackc )
{
	jackc = jack_client_new("aj-connect");

	if (jackc == 0) {
		fprintf(stderr, "Could not become jack client. Maybe jackd isn't running? Aborting!");
		exit(1);
	}
	return jackc;
}

