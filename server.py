#!/usr/bin/env python3
"""
Shortest Path Finder - Local Bridge Server
Invokes the compiled C executable (pathfinder.exe or ./pathfinder) to execute
all graph operations and Dijkstra's algorithm.

Ensures seamless cross-platform local development and guarantees that all calculations
run strictly within the C backend engine.
"""

import http.server
import json
import os
import subprocess
import sys
import urllib.parse

PORT = 8080
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
FRONTEND_DIR = os.path.join(BASE_DIR, "frontend")

def get_c_executable():
    win_bin = os.path.join(BASE_DIR, "pathfinder.exe")
    posix_bin = os.path.join(BASE_DIR, "pathfinder")
    if os.path.exists(win_bin):
        return win_bin
    if os.path.exists(posix_bin):
        return posix_bin
    # Try to compile if not found
    print("[Bridge] Executable not found. Compiling C backend with GCC/Clang...")
    compile_cmd = [
        "gcc", "-Wall", "-Wextra", "-O2", "-Ibackend/include",
        "backend/src/graph.c", "backend/src/minheap.c", "backend/src/dijkstra.c",
        "backend/src/storage.c", "backend/src/server.c", "backend/src/main.c"
    ]
    if sys.platform == "win32":
        compile_cmd.extend(["-lws2_32", "-o", win_bin])
    else:
        compile_cmd.extend(["-o", posix_bin])
    
    subprocess.check_call(compile_cmd, cwd=BASE_DIR)
    return win_bin if sys.platform == "win32" else posix_bin

def run_c_command(args):
    c_bin = get_c_executable()
    cmd = [c_bin] + args
    result = subprocess.run(cmd, cwd=BASE_DIR, capture_output=True, text=True)
    return result.stdout.strip(), result.stderr.strip(), result.returncode

class PathfinderHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=FRONTEND_DIR, **kwargs)

    def end_headers(self):
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        super().end_headers()

    def do_OPTIONS(self):
        self.send_response(204)
        self.end_headers()

    def do_GET(self):
        parsed = urllib.parse.urlparse(self.path)
        path = parsed.path
        query = urllib.parse.parse_qs(parsed.query)

        if path == "/api/health":
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            resp = {
                "status": "ok",
                "bridge": "Python Bridge Subprocess Runner",
                "backend": "Compiled C Binary (Dijkstra Min-Heap)",
                "executable": get_c_executable()
            }
            self.wfile.write(json.dumps(resp).encode("utf-8"))
            return

        if path == "/api/data":
            stdout, stderr, code = run_c_command(["--list"])
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(stdout.encode("utf-8") if stdout else b"{}")
            return

        if path == "/api/shortest-path":
            src = query.get("src", ["1"])[0]
            dest = query.get("dest", ["10"])[0]
            trace_flag = ["--trace"] if query.get("trace", ["0"])[0] == "1" else []
            
            stdout, stderr, code = run_c_command(["--query", src, dest] + trace_flag)
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(stdout.encode("utf-8") if stdout else b"{}")
            return

        if path == "/api/locations/delete":
            loc_id = query.get("id", ["0"])[0]
            stdout, stderr, code = run_c_command(["--del-loc", loc_id])
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(stdout.encode("utf-8"))
            return

        if path == "/api/roads/delete":
            u_id = query.get("u_id", ["0"])[0]
            v_id = query.get("v_id", ["0"])[0]
            stdout, stderr, code = run_c_command(["--del-road", u_id, v_id])
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(stdout.encode("utf-8"))
            return

        # Fallback to static frontend file serving
        return super().do_GET()

    def do_POST(self):
        parsed = urllib.parse.urlparse(self.path)
        path = parsed.path
        length = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(length).decode("utf-8") if length > 0 else ""
        
        # Try JSON body or query string
        params = {}
        if body:
            try:
                params = json.loads(body)
            except Exception:
                params = urllib.parse.parse_qs(body)
                params = {k: v[0] for k, v in params.items()}

        if path == "/api/reset":
            stdout, stderr, code = run_c_command(["--reset"])
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(stdout.encode("utf-8"))
            return

        if path == "/api/locations":
            lid = str(params.get("id", ""))
            code = str(params.get("code", ""))
            name = str(params.get("name", ""))
            x = str(params.get("x", "400"))
            y = str(params.get("y", "300"))
            stdout, stderr, rc = run_c_command(["--add-loc", lid, code, name, x, y])
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(stdout.encode("utf-8"))
            return

        if path == "/api/roads":
            u = str(params.get("u_id", ""))
            v = str(params.get("v_id", ""))
            w = str(params.get("weight", ""))
            b = str(params.get("bidirectional", "1"))
            stdout, stderr, rc = run_c_command(["--add-road", u, v, w, b])
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(stdout.encode("utf-8"))
            return

        self.send_response(404)
        self.end_headers()

def main():
    try:
        c_exe = get_c_executable()
        print(f"[Bridge] Verified C Backend: {c_exe}")
    except Exception as e:
        print(f"[Bridge] Warning: Could not find or compile C binary: {e}")

    server_address = ("", PORT)
    httpd = http.server.HTTPServer(server_address, PathfinderHandler)
    print("==================================================================")
    print(f"  SHORTEST PATH FINDER - HTTP BRIDGE SERVER")
    print(f"  Serving on: http://localhost:{PORT}")
    print(f"  Frontend:   {FRONTEND_DIR}")
    print(f"  C Engine:   Strict C Subprocess Execution")
    print("==================================================================")
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\n[Bridge] Server stopped.")

if __name__ == "__main__":
    main()
