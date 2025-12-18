#!/bin/bash

mkdir /tmp/chat
mkfifo /tmp/chat/main_pipe
mkdir /tmp/chat/receivers
mkdir /tmp/clients
