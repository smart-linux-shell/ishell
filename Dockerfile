FROM python:3.11

WORKDIR /app

RUN apt-get update

# Continue with the rest of the setup
COPY ./app /app/

RUN mkdir -p /data
RUN mkdir -p /data

COPY ./pkg /data


RUN pip3 install --upgrade pip

RUN pip3 install -r /app/requirements.txt

RUN apt-get clean

ENTRYPOINT /app/entrypoint.sh