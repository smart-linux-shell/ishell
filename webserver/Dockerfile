# Use the official Ubuntu base image
FROM ubuntu:22.04

# Install NGINX and OpenSSH server
RUN apt-get update && apt-get install -y \
    nginx \
    openssh-server \
    && rm -rf /var/lib/apt/lists/*

# Create the necessary directories
RUN mkdir -p /var/run/sshd /var/www/html

# Add a default index.html file
RUN echo "<h1>Welcome to NGINX!</h1>" > /var/www/html/index.html

# Set up SSH
RUN useradd -ms /bin/bash docker && \
    echo 'docker:docker' | chpasswd && \
    mkdir -p /home/docker/.ssh && \
    chown -R docker:docker /home/docker/.ssh

# Copy the SSH public key into the container (assumes id_rsa.pub is in the same directory as Dockerfile)
COPY id_rsa.pub /home/docker/.ssh/authorized_keys

# Set permissions for the authorized_keys file
RUN chmod 700 /home/docker/.ssh && \
    chmod 600 /home/docker/.ssh/authorized_keys && \
    chown -R docker:docker /home/docker/.ssh

# Add initial KEY.gpg and sources.list
RUN mkdir /var/www/html/deb
COPY deb/KEY.gpg /var/www/html/deb/KEY.gpg
COPY deb/sources.list /var/www/html/deb/sources.list

# Set permissions for /var/www/html
RUN chown -R docker:docker /var/www/html

# Expose NGINX and SSH ports
EXPOSE 80 22

# Start NGINX and SSH when the container starts
CMD service nginx start && /usr/sbin/sshd -D

