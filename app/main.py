from flask import Flask, send_from_directory, render_template_string, abort
import os
from logger import log
from datetime import datetime

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

# HTML template for directory listing
DIRECTORY_LISTING_TEMPLATE = """
<!DOCTYPE html>
<html>
<head>
    <title>Directory Listing - {{ current_path }}</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        .header { margin-bottom: 20px; }
        .item { padding: 8px; border-bottom: 1px solid #eee; }
        .item:hover { background-color: #f5f5f5; }
        .item a { text-decoration: none; color: #333; }
        .item a:hover { color: #007bff; }
        .directory { font-weight: bold; }
        .file-info { color: #666; float: right; }
    </style>
</head>
<body>
    <div class="header">
        <h2>Directory: {{ current_path }}</h2>
        {% if parent_path %}
        <a href="{{ parent_path }}">&larr; Back to Parent Directory</a>
        {% endif %}
    </div>
    {% for item in items %}
    <div class="item">
        <span class="file-info">{{ item.size }} - {{ item.modified }}</span>
        <a href="{{ item.path }}" class="{{ 'directory' if item.is_dir else '' }}">
            {{ '📁 ' if item.is_dir else '📄 ' }}{{ item.name }}
        </a>
    </div>
    {% endfor %}
</body>
</html>
"""

def get_human_readable_size(size_in_bytes):
    """Convert file size to human readable format"""
    for unit in ['B', 'KB', 'MB', 'GB']:
        if size_in_bytes < 1024:
            return f"{size_in_bytes:.1f} {unit}"
        size_in_bytes /= 1024
    return f"{size_in_bytes:.1f} TB"

def get_directory_listing(base_path, current_path):
    """Generate directory listing information"""
    full_path = os.path.join(base_path, current_path)
    items = []
    
    try:
        for item in os.listdir(full_path):
            item_path = os.path.join(full_path, item)
            stats = os.stat(item_path)
            is_dir = os.path.isdir(item_path)
            
            relative_path = os.path.join(current_path, item)
            if not relative_path.startswith('/'):
                relative_path = '/' + relative_path
                
            items.append({
                'name': item,
                'path': f"/deb{relative_path}",
                'is_dir': is_dir,
                'size': get_human_readable_size(stats.st_size) if not is_dir else '-',
                'modified': datetime.fromtimestamp(stats.st_mtime).strftime('%Y-%m-%d %H:%M:%S')
            })
            
        # Sort items: directories first, then files, both alphabetically
        items.sort(key=lambda x: (not x['is_dir'], x['name'].lower()))
        
        return items
    except Exception as e:
        log.error(f"Error reading directory {full_path}: {str(e)}")
        return []

@app.route('/')
def index():
    return f"{version}. {GUNICORN_VERSION}"

@app.route('/deb/', defaults={'path': ''})
@app.route('/deb/<path:path>')
def serve_deb_repo(path):
    try:
        base_path = '/deb'
        full_path = os.path.join(base_path, path)
        
        # Security check: ensure path is within base directory
        if not os.path.abspath(full_path).startswith(os.path.abspath(base_path)):
            log.error(f"Attempted path traversal: {path}")
            abort(403)
            
        if os.path.isfile(full_path):
            log.info(f"Serving file {full_path}")
            directory = os.path.dirname(full_path)
            filename = os.path.basename(full_path)
            return send_from_directory(directory, filename)
        elif os.path.isdir(full_path):
            log.info(f"Serving directory listing for {full_path}")
            items = get_directory_listing(base_path, path)
            parent_path = '/deb/' + os.path.dirname(path) if path else None
            return render_template_string(
                DIRECTORY_LISTING_TEMPLATE,
                current_path=f"/deb/{path}",
                parent_path=parent_path,
                items=items
            )
        else:
            log.error(f"Path {full_path} does not exist")
            abort(404)
    except Exception as e:
        log.error(f"Error serving path: {str(e)}")
        abort(500)

if __name__ == '__main__':
    log.info('Service started, version: %s', version)
    app.run(debug=True)