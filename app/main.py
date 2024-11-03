from flask import Flask, send_from_directory, render_template_string, abort, url_for
import os
from logger import log
from datetime import datetime
from urllib.parse import quote

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

# HTML template for directory listing with breadcrumb navigation
DIRECTORY_LISTING_TEMPLATE = """
<!DOCTYPE html>
<html>
<head>
    <title>Directory Listing - {{ current_path }}</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        .header { margin-bottom: 20px; }
        .breadcrumb { 
            background-color: #f8f9fa;
            padding: 10px;
            border-radius: 4px;
            margin-bottom: 20px;
        }
        .breadcrumb a {
            color: #007bff;
            text-decoration: none;
            margin: 0 5px;
        }
        .breadcrumb span { color: #666; margin: 0 5px; }
        .item { 
            padding: 8px;
            border-bottom: 1px solid #eee;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }
        .item:hover { background-color: #f5f5f5; }
        .item-name { flex: 2; }
        .item-name a { text-decoration: none; color: #333; }
        .item-name a:hover { color: #007bff; }
        .directory { font-weight: bold; }
        .file-info { 
            flex: 1;
            text-align: right;
            color: #666;
            font-size: 0.9em;
        }
        .size-column { 
            flex: 0.5;
            text-align: right;
            padding-right: 20px;
        }
        .date-column { flex: 1; text-align: right; }
        .header-row {
            font-weight: bold;
            padding: 8px;
            border-bottom: 2px solid #ddd;
            margin-bottom: 10px;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }
    </style>
</head>
<body>
    <div class="header">
        <h2>Repository Browser</h2>
        <div class="breadcrumb">
            <a href="/deb/">Root</a>
            {% for crumb in breadcrumbs %}
                <span>/</span>
                {% if not loop.last %}
                    <a href="{{ crumb.path }}">{{ crumb.name }}</a>
                {% else %}
                    <span>{{ crumb.name }}</span>
                {% endif %}
            {% endfor %}
        </div>
    </div>

    <div class="header-row">
        <div class="item-name">Name</div>
        <div class="size-column">Size</div>
        <div class="date-column">Last Modified</div>
    </div>

    {% if parent_path %}
    <div class="item">
        <div class="item-name">
            <a href="{{ parent_path }}">📁 ..</a>
        </div>
        <div class="size-column">-</div>
        <div class="date-column">-</div>
    </div>
    {% endif %}

    {% for item in items %}
    <div class="item">
        <div class="item-name">
            <a href="{{ item.path }}" class="{{ 'directory' if item.is_dir else '' }}">
                {{ '📁 ' if item.is_dir else '📄 ' }}{{ item.name }}
            </a>
        </div>
        <div class="size-column">{{ item.size }}</div>
        <div class="date-column">{{ item.modified }}</div>
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

def get_breadcrumbs(path):
    """Generate breadcrumb navigation items"""
    if not path:
        return []

    crumbs = []
    current = ""
    parts = path.split('/')
    
    for part in parts:
        if part:
            current = os.path.join(current, part)
            crumbs.append({
                'name': part,
                'path': f'/deb/{current}'
            })
    
    return crumbs

def get_directory_listing(base_path, current_path):
    """Generate directory listing information"""
    full_path = os.path.join(base_path, current_path)
    items = []
    
    try:
        for item in os.scandir(full_path):
            stats = item.stat()
            relative_path = os.path.relpath(item.path, base_path)
            
            items.append({
                'name': item.name,
                'path': f"/deb/{quote(relative_path)}",
                'is_dir': item.is_dir(),
                'size': get_human_readable_size(stats.st_size) if not item.is_dir() else '-',
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
            breadcrumbs = get_breadcrumbs(path)
            
            return render_template_string(
                DIRECTORY_LISTING_TEMPLATE,
                current_path=f"/deb/{path}",
                parent_path=parent_path,
                breadcrumbs=breadcrumbs,
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