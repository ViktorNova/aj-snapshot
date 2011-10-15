/************************************************************************/
/* Copyright (C) 2009-2011 Lieven Moors and Jari Suominen               */
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
/* along with aj-snapshot.  If not, see <http://www.gnu.org/licenses/>. */
/************************************************************************/

#include "aj-snapshot.h"


static void usage(void)
{
    fprintf(stdout, " -------------------------------------------------------------------------------------\n");
    fprintf(stdout, " aj-snapshot: Store/restore JACK and/or ALSA midi connections to/from an xml file     \n");
    fprintf(stdout, "              Copyright (C) 2009-2011 Lieven Moors and Jari Suominen                  \n");
    fprintf(stdout, "                                                                                      \n");
    fprintf(stdout, " Without options -a or -j, all actions apply to both ALSA and JACK connections        \n");
    fprintf(stdout, " Without option -r, and with 'file', aj-snapshot will store connections to file.      \n");
    fprintf(stdout, "                                                                                      \n");
    fprintf(stdout, " Usage: aj-snapshot [-options] [-i client_name] [file]                                \n");
    fprintf(stdout, "                                                                                      \n");
    fprintf(stdout, "  -a,--alsa     Only store/restore ALSA midi connections.                             \n");
    fprintf(stdout, "  -j,--jack     Only store/restore JACK audio and midi connections.                   \n");
    fprintf(stdout, "  -r,--restore  Restore ALSA and/or JACK connections.                                 \n");
    fprintf(stdout, "  -d,--daemon   Restore ALSA and/or JACK connections until terminated.                \n");
    fprintf(stdout, "  -f,--force    Don't ask before overwriting an existing file.                        \n");
    fprintf(stdout, "  -i,--ignore   Specify a name of a client you want to ignore.                        \n");
    fprintf(stdout, "                Note: You can ignore multiple clients by repeating this option.       \n");
    fprintf(stdout, "  -q,--quiet    Be quiet about what happens when storing/restoring connections.       \n");
    fprintf(stdout, "  -x,--remove   With 'file': remove all ALSA and/or JACK connections before restoring.\n");
    fprintf(stdout, "  -x,--remove   Without 'file': only remove ALSA and/or JACK connections.             \n");
    fprintf(stdout, "                                                                                      \n");
} 

enum sys {
    NONE, ALSA, JACK, ALSA_JACK // Bitwise OR of ALSA and JACK is ALSA_JACK
};

enum act {
    REMOVE_ONLY, STORE, RESTORE, DAEMON
};

static const struct option long_option[] = {
    {"help", 0, NULL, 'h'},
    {"alsa", 0, NULL, 'a'},
    {"jack", 0, NULL, 'j'},
    {"restore", 0, NULL, 'r'},
    {"daemon", 0, NULL, 'd'},
    {"remove", 0, NULL, 'x'},
    {"force", 0, NULL, 'f'},
    {"ignore", 1, NULL, 'i'},
    {"quiet", 0, NULL, 'q'},
};

// Nasty globals

// First two are used in signal handlers, so need to be protected with sig_atomic_t.
volatile sig_atomic_t daemon_running = 0; // volatile to prevent compiler optimization in while loop
sig_atomic_t reload_xml = 0;

int verbose = 1;
int jack_dirty = 0;
int ic_n = 0; // number of ignored clients.
char *ignored_clients[IGNORED_CLIENTS_MAX]; // array to store names of ignored clients
int exit_success = 1; 

pthread_mutex_t graph_order_callback_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t shutdown_callback_lock = PTHREAD_MUTEX_INITIALIZER;

//////////////////////

int is_ignored_client(const char *name) // function to check if string is name of ignored client.
{
    int i;
    for(i = 0; i < ic_n; i++){
        if( strcmp(name, ignored_clients[i]) == 0) return 1;
    }
    return 0;
}

void exit_cli(int sig) {
    if (verbose) {
        fprintf(stdout, "\raj-snapshot: Exiting!\n");
    }
    daemon_running = 0;
}

void reload_xml_file(int sig) {
    reload_xml = 1;
}

int main(int argc, char **argv)
{ 
    int c;
    int try_remove = 0;
    int remove_connections = 0;
    int force = 0;
    enum sys system = NONE;
    enum sys system_ready = NONE; // When we asked for a system and got it (bitwise or'd).
    enum act action = STORE;
    static const char *filename;
    snd_seq_t* seq = NULL;
    jack_client_t* jackc = NULL;
    mxml_node_t* xml_node = NULL;
	

    while ((c = getopt_long(argc, argv, "ajrdxfi:qh", long_option, NULL)) != -1) {

        switch (c){

        case 'a':
            system |= ALSA;
            break;
        case 'j':
            system |= JACK;
            break;
        case 'r':
            action = RESTORE;
            break;
        case 'd':
            action = DAEMON;
            break;
        case 'x':
            try_remove = 1;
            break;
        case 'f':
            force = 1;
            break;
        case 'i':
            if( ic_n < (IGNORED_CLIENTS_MAX - 1) )
                ignored_clients[ic_n++] = optarg;
            else {
                fprintf(stderr, "aj-snapshot: ERROR: you have more then %i ignored clients\n", IGNORED_CLIENTS_MAX);
                exit(EXIT_FAILURE);
            }
            break;
        case 'q':
            verbose = 0;
            break;
        case 'h':
            usage();
            return 1;
        default:
            usage();
            return 1;
        }
    }

    if (system == NONE) system = ALSA_JACK; // Default when no specific system has been specified.

    if (argc == (optind + 1)) { // If something is left after parsing options, it must be the file
        filename = argv[optind];
    }
    else if ((argc == optind) && try_remove){ 
        action = REMOVE_ONLY; // To make sure action is not STORE or RESTORE
    }
    else {
        fprintf(stderr, " -------------------------------------------------------------------\n");
        fprintf(stderr, " aj-snapshot: Please specify one file to store/restore the snapshot,\n");
        fprintf(stderr, " or use option -x to remove connections only\n");
        usage();
        return 1;
    }
	
    if(try_remove){
        if (action != STORE){
            remove_connections = 1;
        }
        else {
            if(verbose) fprintf(stderr, "aj-snapshot: Will not remove connections before storing connections\n");
        }
	}

    if (action == DAEMON) {
        struct sigaction sig_int_handler;
        struct sigaction sig_hup_handler;

        sig_int_handler.sa_handler = exit_cli;
        sigemptyset(&sig_int_handler.sa_mask);
        sig_int_handler.sa_flags = 0;
        sig_hup_handler.sa_handler = reload_xml_file;
        sigemptyset(&sig_hup_handler.sa_mask);
        sig_hup_handler.sa_flags = 0;

        sigaction(SIGINT, &sig_int_handler, NULL);
        sigaction(SIGTERM, &sig_int_handler, NULL);
        sigaction(SIGHUP, &sig_hup_handler, NULL);
    }

    // Get XML node first:

    switch (action){
        case STORE:
            xml_node = mxmlNewXML("1.0");
            break;
        case RESTORE:
        case DAEMON:
            xml_node = read_xml(filename, xml_node);
            break;
    }

    // Initialize clients with ALSA and JACK, and remove connections if necessary.

    if ((system & ALSA) == ALSA) {
        seq = alsa_initialize(seq);
        if(seq){
            system_ready |= ALSA;
            if(remove_connections){
                alsa_remove_connections(seq);
                VERBOSE("aj-snapshot: all ALSA connections removed!\n");
            }
        } 
    }
    if ((system & JACK) == JACK) {
        jack_initialize(&jackc, (action == DAEMON));
        if(jackc){
            system_ready |= JACK;
            if (remove_connections) {
                jack_remove_connections(jackc);
                VERBOSE("aj-snapshot: all JACK connections removed!\n");
            }
        } 
    }

    if(action != DAEMON){
        // If not in daemon mode, store/restore snapshots
        if ((system_ready & ALSA) == ALSA) {
            switch (action){
                case STORE:
                    alsa_store(seq, xml_node);
                    break;
                case RESTORE:
                    alsa_restore(seq, xml_node);
                    break;
            }
        }

        if ((system_ready & JACK) == JACK) {
            switch (action){
                case STORE:
                    jack_store(jackc, xml_node);
                    break;
                case RESTORE:
                    jack_restore(&jackc, xml_node);
                    break;
            }
        }

        // Write file when storing:
        if(action == STORE){
            write_xml(filename, xml_node, force);
        }
    }
    else {
        // Run Daemon
        daemon_running = 1;
        while (daemon_running) {
            if (reload_xml > 0) { // Reload XML if triggered with HUP signal
                reload_xml = 0;
                xml_node = read_xml(filename, xml_node);
                if ((system_ready & ALSA) == ALSA) {
                    if(remove_connections){ 
                        alsa_remove_connections(seq);
                    }
                }
                if ((system_ready & JACK) == JACK) {
                    if(remove_connections){ 
                        jack_remove_connections(jackc);
                    }
                }
            }
            if ((system_ready & ALSA) == ALSA) {
                alsa_restore(seq, xml_node);
            }
            if ((system & JACK) == JACK) {
                if (jackc == NULL) { // Make sure jack is up.
                    jack_initialize(&jackc, (action==DAEMON));
                }	
                pthread_mutex_lock( &graph_order_callback_lock );

                if (jack_dirty > 0) { // Only restore when connections have changed
                    jack_dirty = 0;
                    pthread_mutex_unlock( &graph_order_callback_lock );
                    jack_restore(&jackc, xml_node);
                }
                else pthread_mutex_unlock( &graph_order_callback_lock );
            }
            usleep(POLLING_INTERVAL_MS * 1000);
        }
    }

    // Cleanup xml_node:
    if(xml_node) mxmlDelete(xml_node);

    // Close ALSA and JACK clients if necessary.
    if (seq != NULL) snd_seq_close(seq);
    if (jackc != NULL) jack_client_close(jackc);

    // Exit

    if (action == DAEMON || exit_success) {
        exit(EXIT_SUCCESS);
    }
    exit(EXIT_FAILURE);
}













/*
    switch (system) {
        case ALSA:
            //seq = alsa_initialize(seq);
            if(remove_connections){
                 if (seq != NULL) {
                    alsa_remove_connections(seq);
                    if(verbose) fprintf(stdout, "aj-snapshot: all ALSA connections removed!\n");
                } else {
                    restore_successful = 0;
                    if(verbose) fprintf(stdout, "aj-snapshot: Did NOT remove ALSA connections!\n");
                }
            }
            switch (action){
                case STORE:
                    if (seq == NULL) {
                        if(verbose) fprintf(stdout, "aj-snapshot: Did NOT store ALSA connections!\n");
                        restore_successful = 0;
                        break;
                    }
                    xml_node = mxmlNewXML("1.0");
                    alsa_store(seq, xml_node);
                    if( write_xml(filename, xml_node, force) ){
                        if(verbose) fprintf(stdout, "aj-snapshot: ALSA connections stored!\n");
                    } else if(verbose) fprintf(stdout, "aj-snapshot: Did NOT store ALSA connections!\n");
                    break;
                case RESTORE:
                    if (seq == NULL) {
                        if(verbose) fprintf(stdout, "aj-snapshot: Skip restoring ALSA connections.\n");
                        break;
                    }
                    xml_node = read_xml(filename, xml_node);
                    alsa_restore(seq, xml_node);
                    if(verbose) {
                        if (restore_successful) {
                            fprintf(stdout, "aj-snapshot: SUCCESSFUL snapshot restore!\n");
                        } else {
                            fprintf(stdout, "aj-snapshot: All ALSA connections could not be restored!\n");
                        }
                    }
                    break;
                case REMOVE_ONLY:
                    break;
                case DAEMON:
                    xml_node = read_xml(filename, xml_node);
                    if(verbose) fprintf(stdout, "aj-snapshot: ALSA connections monitored!\n");
                    daemon_running = 1;
                    while (daemon_running) {
                        if (reload_xml > 0) {
                            reload_xml = 0;
                            if(verbose) fprintf(stdout, "reloading XML file: %s!\n", filename);
                            xml_node = read_xml(filename, xml_node);
                            if(remove_connections){
                                alsa_remove_connections(seq);
                                if(verbose) fprintf(stdout, "aj-snapshot: all ALSA connections removed!\n");
                            }
                        }
                        alsa_restore(seq, xml_node);
                        usleep(POLLING_INTERVAL_MS * 1000);
                    }
                    break;
            }
            if (seq != NULL) snd_seq_close(seq);
            break;
        case JACK:
            //jack_initialize(&jackc, (action==DAEMON));
            if(remove_connections){
                if (jackc!=NULL) {
                    jack_remove_connections(jackc);
                    if(verbose) fprintf(stdout, "aj-snapshot: all JACK connections removed!\n");
                } else {
                    restore_successful = 0;
                    if(verbose) fprintf(stdout, "aj-snapshot: Did NOT remove JACK connections!\n");
                }
            }
            switch (action){
                case STORE:
                    if (jackc==NULL) {
                        if(verbose) fprintf(stdout, "aj-snapshot: Did NOT store JACK connections!\n");
                        restore_successful = 0;
                        break;
                    }
                    xml_node = mxmlNewXML("1.0");
                    jack_store(jackc, xml_node);
                    if( write_xml(filename, xml_node, force) ){
                        if(verbose) fprintf(stdout, "aj-snapshot: JACK connections stored!\n");
                    } else if (verbose) {
                        fprintf(stdout, "aj-snapshot: Did NOT store JACK connections!\n");
                    }
                    mxmlDelete(xml_node);
                    break;
                case RESTORE:
                    if (jackc == NULL) {
                        if(verbose) fprintf(stdout, "aj-snapshot: Skip restoring JACK connections.\n");
                        break;
                    }
                    xml_node = read_xml(filename, xml_node);
                    jack_restore(&jackc, xml_node);
                    mxmlDelete(xml_node);
                    if (verbose) {
                        if(restore_successful){ 
                            fprintf(stdout, "aj-snapshot: SUCCESSFUL snapshot restore!\n");
                        } else {
                            fprintf(stdout, "aj-snapshot: All JACK connections could NOT be restored!\n");
                        }
                    }
                    break;
                case REMOVE_ONLY:
                    break;
                case DAEMON:			
                    xml_node = read_xml(filename, xml_node);
                    if(verbose) fprintf(stdout, "aj-snapshot: JACK connections monitored!\n");
                    daemon_running = 1;
                    while (daemon_running) {
                        if (jackc == NULL) {
                            jack_initialize(&jackc, (action==DAEMON));
                        }	
                        if (reload_xml > 0) {
                            reload_xml = 0;
                            if(verbose) fprintf(stdout, "reloading XML file: %s!\n", filename);
                            xml_node = read_xml(filename, xml_node);
			                if(remove_connections){
                                jack_remove_connections(jackc);
                                if(verbose) fprintf(stdout, "aj-snapshot: all JACK connections removed!\n");
                            }
                        }
                        pthread_mutex_lock( &callback_lock );
                        if (jack_dirty > 0) {
                            jack_dirty = 0;
                            pthread_mutex_unlock( &callback_lock );
                            jack_restore(&jackc, xml_node);
                        } 
                        else pthread_mutex_unlock( &callback_lock );
                        usleep(POLLING_INTERVAL_MS * 1000);
                    }
                    mxmlDelete(xml_node);
                    break;
            }
            if (jackc != NULL) jack_client_close(jackc);
            break;           
        case ALSA_JACK:
            //jack_initialize(&jackc, (action==DAEMON));
            //seq = alsa_initialize(seq);
            if(remove_connections){
                if (jackc!=NULL && seq != NULL) {
                    alsa_remove_connections(seq);
                    jack_remove_connections(jackc);
                    if(verbose) fprintf(stdout, "aj-snapshot: all ALSA & JACK connections removed!\n");
                } else {
                    if(verbose) fprintf(stdout, "aj-snapshot: Did NOT remove ALSA & JACK connections!\n");
                    restore_successful = 0;
                }
            }
            switch (action){
                case STORE:
                    if (jackc==NULL || seq == NULL) {
                        if(verbose) fprintf(stdout, "aj-snapshot: Did NOT store ALSA & JACK connections!\n");
                        restore_successful = 0;
                        break;
                    }
                    xml_node = mxmlNewXML("1.0");
                    alsa_store(seq, xml_node);
                    jack_store(jackc, xml_node);
                    if( write_xml(filename, xml_node, force) ){
                        if(verbose) fprintf(stdout, "aj-snapshot: ALSA & JACK connections stored!\n");
                    } else if(verbose) fprintf(stdout, "aj-snapshot: Did NOT store ALSA and JACK connections!\n");
                    mxmlDelete(xml_node);
                    break;
                case RESTORE:
                    if (jackc == NULL || seq == NULL) {
                        if(verbose) fprintf(stdout, "aj-snapshot: Skip restoring JACK connections.\n");
                        if(verbose) fprintf(stdout, "aj-snapshot: Skip restoring ALSA connections.\n");
                        restore_successful = 0;
                        break;
                    }
                    xml_node = read_xml(filename, xml_node);
                    alsa_restore(seq, xml_node);
                    jack_restore(&jackc, xml_node);
                    mxmlDelete(xml_node);
                    if (verbose) {
                        if(restore_successful){ 
                            fprintf(stdout, "aj-snapshot: SUCCESSFUL snapshot restore!\n");
                        } else {
                            fprintf(stdout, "aj-snapshot: All ALSA & JACK connections could NOT be restored!\n");
                        }
                    }
                    break;
                case REMOVE_ONLY:
                    break;
                case DAEMON:
                    xml_node = read_xml(filename, xml_node);
                    if(verbose) fprintf(stdout, "aj-snapshot: ALSA & JACK connections monitored!\n");
                    daemon_running = 1;
                    while (daemon_running) {
                        if (jackc == NULL) {
                            // We should probably wait a moment after jack has closed before trying to do this again?
                            jack_initialize(&jackc, (action==DAEMON));
                        }

                        if (reload_xml > 0) {
                            reload_xml = 0;
                            if(verbose) fprintf(stdout, "reloading XML file: %s!\n", filename);
                            xml_node = read_xml(filename, xml_node);
                            if(remove_connections){
                                alsa_remove_connections(seq);
                                jack_remove_connections(jackc);
                                if(verbose) fprintf(stdout, "aj-snapshot: all ALSA & JACK connections removed!\n");
                            }
                        }

                        alsa_restore(seq, xml_node);

                        pthread_mutex_lock( &callback_lock );
                        if (jack_dirty > 0) {
                            jack_dirty = 0;
                            pthread_mutex_unlock( &callback_lock );
                            jack_restore(&jackc, xml_node);
                        } 
                        else pthread_mutex_unlock( &callback_lock );
                        
                        usleep(POLLING_INTERVAL_MS * 1000);
                    }
                    mxmlDelete(xml_node);
                    break;
            }
            if (seq != NULL) snd_seq_close(seq);
            if (jackc != NULL) jack_client_close(jackc);
            break;
    }
    if (action==DAEMON || restore_successful) {
        exit(EXIT_SUCCESS);
    }
    exit(EXIT_FAILURE);
}*/
