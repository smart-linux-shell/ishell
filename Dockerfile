FROM python:3.11

WORKDIR /app

COPY ./app /app/

RUN mkdir -p /deb
COPY ./pkg/*/ /deb/

RUN apt-get update

RUN pip3 install --upgrade pip
RUN pip3 install -r /app/requirements.txt

RUN apt-get clean

ENTRYPOINT /app/entrypoint.sh