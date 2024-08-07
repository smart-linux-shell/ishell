# Use the official Ubuntu base image
FROM nginx:latest

# Add deb folder
COPY ./deb/ /usr/share/nginx/html/deb/

# Expose NGINX
EXPOSE 80

CMD [ "nginx", "-g", "daemon off;"]
