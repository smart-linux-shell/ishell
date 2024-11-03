#!/bin/bash

cd /app
#python3 -m flask --app ./main.py run --host=0.0.0.0 --port=5000

export GUNICORN_VERSION=$(gunicorn --version)
gunicorn -w 1  -b 0.0.0.0:5000 main:app
