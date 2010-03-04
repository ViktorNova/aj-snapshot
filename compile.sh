#!/bin/bash

gcc -Wall aj-snapshot.c aj-alsa.c -l asound -l mxml -o aj-snapshot
