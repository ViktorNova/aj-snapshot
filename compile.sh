#!/bin/bash

gcc -Wall aj-snapshot.c aj-alsa.c aj-file.c aj-jack.c aj-remove.c -l asound -l jack -l mxml -o aj-snapshot;

exit 0;
