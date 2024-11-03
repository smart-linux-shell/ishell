#!/bin/bash

cd /app

export GUNICORN_VERSION=$(gunicorn --version)
gunicorn -w 1  -b 0.0.0.0:5000 main:app
