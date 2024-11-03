FROM python:3.11

WORKDIR /app

RUN apt-get update

# Update apt and install the ishell package
RUN apt-get update && \
    apt-get install -y ishell && \
    apt-get clean

# Continue with the rest of the setup
COPY ./app /app/

RUN mkdir -p /data/deb


# Copy the deb package files first
COPY ./pkg/deb /data/deb/


RUN pip3 install --upgrade pip

RUN pip3 install -r /app/requirements.txt

RUN apt-get clean

ENTRYPOINT /app/entrypoint.sh