#!/bin/bash

docker build -t nginx-ssh .
docker run -d --name nginx-ssh -p 80:80 -p 2222:22 -v www-data:/var/www/html nginx-ssh
