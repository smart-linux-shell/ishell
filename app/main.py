from flask import Flask
import os
from logger import log

# read version from file if exists
version = "unknown"
try:
    with open("/VERSION") as f:
        version = f.read()
except FileNotFoundError:
    pass

app = Flask(__name__)
GUNICORN_VERSION=f"{os.getenv('GUNICORN_VERSION', 'Unknown')}"
log.info('Service started, version: [%s]', GUNICORN_VERSION)

@app.route('/')
def index():
    return f"{version}. {GUNICORN_VERSION}"

if __name__ == '__main__':
    log.info('Service started, version: %s', version)
    app.run(debug=True)
