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
#include "aj-alsa.h"
#include "aj-file.h"
#include "aj-jack.h"
#include "aj-remove.h"
#include "signal.h"
#include "pthread.h"

#define POLLING_INTERVAL_MS 200

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
    ALSA, JACK, ALSA_JACK
};

enum act {
    STORE, RESTORE, REMOVE_ONLY, DAEMON
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
int verbose = 1;
int daemon_running = 0;
int jack_dirty = 1;
int restore_successful = 1; 
int reload_xml = 0;
pthread_mutex_t callback_lock = PTHREAD_MUTEX_INITIALIZER;

char *ignored_clients[IGNORED_CLIENTS_MAX]; // array to store names of ignored clients

int ic_n = 0; // number of ignored clients.

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
        fprintf(stdout, "\raj-snapshot exiting!\n");
    }
    daemon_running = 0;
}

void reload_xml_file(int sig) {
    if (verbose) {
        fprintf(stdout, "\rreloading xml!\n");
    }
    reload_xml = 1;
}

int main(int argc, char **argv)
{ 
    int c;
    int try_remove = 0;
    int remove_connections = 0;
    int force = 0;
    enum sys system = ALSA_JACK;
    enum act action = STORE;
    static const char *filename;
    snd_seq_t* seq = NULL;
    jack_client_t* jackc = NULL;
    mxml_node_t* xml_node = NULL;
	

    while ((c = getopt_long(argc, argv, "ajrdxfi:qh", long_option, NULL)) != -1) {

        switch (c){

        case 'a':
            if(system == JACK) system = ALSA_JACK;
            else system = ALSA;
            break;
        case 'j':
            if(system == ALSA) system = ALSA_JACK;
            else system = JACK;
            break;
        case 'r':
            action = RESTORE;
            break;
        case 'd':
            action = DAEMON;
            daemon_running = 1;
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

    if (argc == (optind + 1)) {
        filename = argv[optind];
    }
    else if ((argc == optind) && try_remove){
        action = REMOVE_ONLY;
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
        else if(verbose) fprintf(stderr, "aj-snapshot: Will not remove connections before storing connections\n");
	}

    if (action==DAEMON) {
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

    switch (system) {
        case ALSA:
            seq = alsa_initialize(seq);
            if(remove_connections){
                alsa_remove_connections(seq);
                if(verbose) fprintf(stdout, "aj-snapshot: all ALSA connections removed!\n");
            }
            switch (action){
                case STORE:
                    xml_node = mxmlNewXML("1.0");
                    alsa_store(seq, xml_node);
                    if( write_xml(filename, xml_node, force) ){
                        if(verbose) fprintf(stdout, "aj-snapshot: ALSA connections stored!\n");
                    } else if(verbose) fprintf(stdout, "aj-snapshot: Did NOT store ALSA connections!\n");
                    break;
                case RESTORE:
                    xml_node = read_xml(filename, xml_node);
                    alsa_restore(seq, xml_node);
                    if(verbose) {
                        if (restore_successful) {
                            fprintf(stdout, "aj-snapshot: ALSA connections restored!\n");
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
                    while (daemon_running) {
                        if (reload_xml > 0) {
                            reload_xml = 0;
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
            snd_seq_close(seq);
            break;
        case JACK:
            jackc = jack_initialize(jackc, (action==DAEMON));
            if(remove_connections){
                jack_remove_connections(jackc);
                if(verbose) fprintf(stdout, "aj-snapshot: all JACK connections removed!\n");
            }
            switch (action){
                case STORE:
                    xml_node = mxmlNewXML("1.0");
                    jack_store(jackc, xml_node);
                    if( write_xml(filename, xml_node, force) ){
                        if(verbose) fprintf(stdout, "aj-snapshot: JACK connections stored!\n");
                    } else if(verbose) fprintf(stdout, "aj-snapshot: Did NOT store JACK connections!\n");
                    mxmlDelete(xml_node);
                    break;
                case RESTORE:
                    xml_node = read_xml(filename, xml_node);
                    jack_restore(jackc, xml_node);
                    mxmlDelete(xml_node);
                    if (restore_successful) {
                            fprintf(stdout, "aj-snapshot: JACK connections restored!\n");
                        } else {
                            fprintf(stdout, "aj-snapshot: All JACK connections could not be restored!\n");
                        }
                    break;
                case REMOVE_ONLY:
                    break;
                case DAEMON:					
                    xml_node = read_xml(filename, xml_node);
                    if(verbose) fprintf(stdout, "aj-snapshot: JACK connections monitored!\n");
                    while (daemon_running) {
                        if (reload_xml > 0) {
                            reload_xml = 0;
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
                            jack_restore(jackc, xml_node);
                        } 
                        else pthread_mutex_unlock( &callback_lock );
                        usleep(POLLING_INTERVAL_MS * 1000);
                    }
                    mxmlDelete(xml_node);
                    break;
            }
            break;
            jack_client_close(jackc);
        case ALSA_JACK:
            jackc = jack_initialize(jackc, (action==DAEMON));
            seq = alsa_initialize(seq);
            if(remove_connections){
                alsa_remove_connections(seq);
                jack_remove_connections(jackc);
                if(verbose) fprintf(stdout, "aj-snapshot: all ALSA & JACK connections removed!\n");
            }
            switch (action){
                case STORE:
                    xml_node = mxmlNewXML("1.0");
                    alsa_store(seq, xml_node);
                    jack_store(jackc, xml_node);
                    if( write_xml(filename, xml_node, force) ){
                        if(verbose) fprintf(stdout, "aj-snapshot: ALSA & JACK connections stored!\n");
                    } else if(verbose) fprintf(stdout, "aj-snapshot: Did NOT store ALSA and JACK connections!\n");
                    mxmlDelete(xml_node);
                    break;
                case RESTORE:
                    xml_node = read_xml(filename, xml_node);
                    alsa_restore(seq, xml_node);
                    jack_restore(jackc, xml_node);
                    mxmlDelete(xml_node);
                    if (restore_successful) {
                        fprintf(stdout, "aj-snapshot: ALSA & JACK connections restored!\n");
                    } else {
                        fprintf(stdout, "aj-snapshot: All ALSA & JACK connections could not be restored!\n");
                    }
                    break;
                case REMOVE_ONLY:
                    break;
                case DAEMON:
                    xml_node = read_xml(filename, xml_node);
                    if(verbose) fprintf(stdout, "aj-snapshot: ALSA & JACK connections monitored!\n");
                    while (daemon_running) {
                        if (reload_xml > 0) {
                            reload_xml = 0;
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
                            jack_restore(jackc, xml_node);
                        } 
                        else pthread_mutex_unlock( &callback_lock );
                        usleep(POLLING_INTERVAL_MS * 1000);
                    }
                    mxmlDelete(xml_node);
                    break;
            }
            snd_seq_close(seq);
            jack_client_close(jackc);
            break;
    }
    if (action==DAEMON || restore_successful) {
        exit(EXIT_SUCCESS);
    }
    exit(EXIT_FAILURE);
}
