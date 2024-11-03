from flask import Flask, Response
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
        if not is_allowed_file(filename):
            abort(403)
            
        path = os.path.join(app.static_folder, 'deb', filename)
        
        def generate():
            with open(path, 'rb') as f:
                while chunk := f.read(8192):
                    yield chunk
                    
        return Response(
            generate(),
            mimetype=mimetypes.guess_type(filename)[0],
            direct_passthrough=True
        )
    except FileNotFoundError:
        abort(404)

if __name__ == '__main__':
    log.info('Service started, version: %s', version)
    app.run(debug=True)
