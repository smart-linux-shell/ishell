FROM python:3.11

WORKDIR /app

RUN apt-get update


COPY ./app /app/
RUN mkdir -p /data

RUN pip3 install --upgrade pip

RUN pip3 install -r /app/requirements.txt

RUN apt-get clean

ENTRYPOINT /app/entrypoint.sh

