from flask import Flask, send_from_directory
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

@app.route('/deb/<path:filename>')
def serve_deb_repo(filename):
    try:
        filepath = os.path.join('/deb', filename)
        if os.path.exists(filepath):
            log.info(f"Serving file {filepath}.")
            # Get the directory path and actual filename
            directory = os.path.dirname(os.path.join('/data', filename))
            basename = os.path.basename(filename)
            return send_from_directory(directory, basename)
        else:
            log.error(f"File {filepath} does not exist.")
            abort(404)
    except Exception as e:
        log.error(f"Error serving file: {str(e)}")
        return "Invalid"

if __name__ == '__main__':
    log.info('Service started, version: %s', version)
    app.run(debug=True)
