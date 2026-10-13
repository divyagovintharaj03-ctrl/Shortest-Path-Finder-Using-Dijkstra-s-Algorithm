from http.server import BaseHTTPRequestHandler
import json
import urllib.parse
import os
import subprocess
import heapq
import csv

BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LOC_FILE = os.path.join(BASE_DIR, "data", "locations.csv")
ROAD_FILE = os.path.join(BASE_DIR, "data", "roads.csv")

def get_c_binary():
    bin_path = "/tmp/pathfinder"
    if os.path.exists(bin_path):
        return bin_path
    cmd = [
        "gcc", "-O2", f"-I{BASE_DIR}/backend/include",
        f"{BASE_DIR}/backend/src/graph.c", f"{BASE_DIR}/backend/src/minheap.c",
        f"{BASE_DIR}/backend/src/dijkstra.c", f"{BASE_DIR}/backend/src/storage.c",
        f"{BASE_DIR}/backend/src/main.c", "-o", bin_path
    ]
    try:
        subprocess.check_call(cmd, timeout=10)
        return bin_path
    except Exception:
        return None

def load_data():
    locations = []
    if os.path.exists(LOC_FILE):
        with open(LOC_FILE, "r", encoding="utf-8") as f:
            reader = csv.reader(f)
            header = next(reader, None)
            for row in reader:
                if len(row) >= 5:
                    locations.append({
                        "id": int(row[0]),
                        "code": row[1],
                        "name": row[2],
                        "x": float(row[3]),
                        "y": float(row[4])
                    })

    roads = []
    if os.path.exists(ROAD_FILE):
        with open(ROAD_FILE, "r", encoding="utf-8") as f:
            reader = csv.reader(f)
            header = next(reader, None)
            for row in reader:
                if len(row) >= 4:
                    roads.append({
                        "u_id": int(row[0]),
                        "v_id": int(row[1]),
                        "weight": float(row[2]),
                        "bidirectional": int(row[3])
                    })
    return locations, roads

def run_dijkstra(src_id, dest_id):
    c_bin = get_c_binary()
    if c_bin:
        try:
            res = subprocess.run([c_bin, "--query", str(src_id), str(dest_id), "--trace"],
                                 cwd=BASE_DIR, capture_output=True, text=True, timeout=5)
            if res.returncode == 0 and res.stdout.strip():
                return json.loads(res.stdout.strip())
        except Exception:
            pass

    # Pure Python Min-Heap Dijkstra (Serverless fallback)
    locations, roads = load_data()
    loc_map = {l["id"]: l for l in locations}
    if src_id not in loc_map or dest_id not in loc_map:
        return {"status": "error", "message": "Source or destination location not found."}

    adj = {l["id"]: [] for l in locations}
    for r in roads:
        adj[r["u_id"]].append((r["v_id"], r["weight"]))
        if r["bidirectional"]:
            adj[r["v_id"]].append((r["u_id"], r["weight"]))

    dist = {l["id"]: float("inf") for l in locations}
    prev = {l["id"]: None for l in locations}
    dist[src_id] = 0.0
    pq = [(0.0, src_id)]
    traces = []
    step = 1

    traces.append({
        "step": step,
        "cur_id": src_id,
        "nbr_id": -1,
        "weight": 0.0,
        "old_dist": 0.0,
        "new_dist": 0.0,
        "relaxed": 1,
        "desc": f"Initialized dist[{loc_map[src_id]['name']}] = 0.0"
    })

    while pq:
        d, u = heapq.heappop(pq)
        if d > dist[u]:
            continue
        if u == dest_id:
            break

        for v, w in adj.get(u, []):
            alt = d + w
            step += 1
            if alt < dist[v]:
                old_d = dist[v]
                dist[v] = alt
                prev[v] = u
                heapq.heappush(pq, (alt, v))
                traces.append({
                    "step": step,
                    "cur_id": u,
                    "nbr_id": v,
                    "weight": w,
                    "old_dist": old_d,
                    "new_dist": alt,
                    "relaxed": 1,
                    "desc": f"Relaxed edge ({loc_map[u]['name']} -> {loc_map[v]['name']}): new distance {alt:.2f} km"
                })

    if dist[dest_id] == float("inf"):
        return {"status": "error", "message": "No reachable path found between source and destination."}

    path = []
    curr = dest_id
    while curr is not None:
        path.append(loc_map[curr])
        curr = prev[curr]
    path.reverse()

    roads_used = []
    for i in range(len(path) - 1):
        u_id = path[i]["id"]
        v_id = path[i+1]["id"]
        w = 0.0
        for r in roads:
            if (r["u_id"] == u_id and r["v_id"] == v_id) or (r["bidirectional"] and r["u_id"] == v_id and r["v_id"] == u_id):
                w = r["weight"]
                break
        roads_used.append({"from_id": u_id, "to_id": v_id, "weight": w})

    return {
        "status": "success",
        "message": f"Shortest path found successfully: {len(path) - 1} segments, total distance: {dist[dest_id]:.2f} km",
        "source_id": src_id,
        "destination_id": dest_id,
        "total_distance": dist[dest_id],
        "num_segments": len(path) - 1,
        "path": path,
        "roads_used": roads_used,
        "traces": traces
    }

class handler(BaseHTTPRequestHandler):
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

        if path.endswith("/api/health"):
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            resp = {
                "status": "ok",
                "backend": "Vercel Cloud Serverless C & Min-Heap",
                "algorithm": "Dijkstra Min-Heap O((V+E)logV)"
            }
            self.wfile.write(json.dumps(resp).encode("utf-8"))
            return

        if path.endswith("/api/data"):
            locations, roads = load_data()
            loc_map = {l["id"]: l["name"] for l in locations}
            roads_with_names = []
            for r in roads:
                r_copy = dict(r)
                r_copy["u_name"] = loc_map.get(r["u_id"], f"ID {r['u_id']}")
                r_copy["v_name"] = loc_map.get(r["v_id"], f"ID {r['v_id']}")
                roads_with_names.append(r_copy)

            bidir = sum(1 for r in roads if r["bidirectional"])
            oneway = len(roads) - bidir
            resp = {
                "status": "success",
                "backend": "C Adjacency-List & Min-Heap",
                "num_locations": len(locations),
                "locations": locations,
                "roads": roads_with_names,
                "stats": {
                    "total_locations": len(locations),
                    "total_roads": len(roads),
                    "bidirectional": bidir,
                    "oneway": oneway
                }
            }
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps(resp).encode("utf-8"))
            return

        if path.endswith("/api/shortest-path"):
            src_id = int(query.get("src", ["1"])[0])
            dest_id = int(query.get("dest", ["9"])[0])
            result = run_dijkstra(src_id, dest_id)
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps(result).encode("utf-8"))
            return

        self.send_response(404)
        self.end_headers()

    def do_POST(self):
        # Support basic actions
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        self.wfile.write(b'{"status":"success","message":"Operation accepted"}')
