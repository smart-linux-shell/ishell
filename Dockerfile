FROM python:3.11

WORKDIR /app

RUN apt-get update

# Create directory for deb packages
RUN mkdir -p /app/pkg/deb

# Copy the deb package files first
COPY ./pkg/deb /app/pkg/deb/

# Add the GPG key and configure the repository
RUN apt-get install -y gnupg
COPY ./pkg/deb/KEY.gpg /etc/apt/trusted.gpg.d/ishell.gpg
COPY ./pkg/deb/sources.list /etc/apt/sources.list.d/ishell.list

# Update apt and install the ishell package
RUN apt-get update && \
    apt-get install -y ishell && \
    apt-get clean

# Continue with the rest of the setup
COPY ./app /app/
RUN mkdir -p /data

RUN pip3 install --upgrade pip

RUN pip3 install -r /app/requirements.txt

RUN apt-get clean

ENTRYPOINT /app/entrypoint.sh