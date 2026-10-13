/**
 * @file real_map.js
 * @brief Interactive Real-World Map using Leaflet.js & OpenStreetMap / CartoDB
 *
 * Integrates real geographical coordinates (Latitude, Longitude) with the C Dijkstra backend.
 * Features:
 * - Real world street tiles with Light Theme styling
 * - Custom pins for locations (Start: Green, Destination: Red, Route: Violet, Neutral: Slate)
 * - Real road polylines with weight labels
 * - Interactive map clicking to drop new real-world location markers
 * - Automatic bounding-box zoom on calculated shortest paths
 */

class RealWorldMap {
  constructor(mapContainerId) {
    this.containerId = mapContainerId;
    this.map = null;
    this.markers = new Map();
    this.roadPolylines = [];
    this.pathPolyline = null;
    this.locations = [];
    this.roads = [];
    this.sourceId = null;
    this.destId = null;
    this.activePath = null;

    this.initMap();
  }

  initMap() {
    const el = document.getElementById(this.containerId);
    if (!el) return;

    // Center initially on South India / Chennai or global default
    this.map = L.map(this.containerId, {
      zoomControl: true,
      attributionControl: true
    }).setView([13.04, 80.22], 12);

    // Clean, modern Light Theme tiles from CartoDB Positron
    L.tileLayer('https://{s}.basemaps.cartocdn.com/light_all/{z}/{x}/{y}{r}.png', {
      attribution: '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors &copy; <a href="https://carto.com/attributions">CARTO</a>',
      subdomains: 'abcd',
      maxZoom: 19
    }).addTo(this.map);

    // Allow user to click anywhere on the real world map to drop a new location pin
    this.map.on('click', (e) => {
      const lat = e.latlng.lat.toFixed(5);
      const lng = e.latlng.lng.toFixed(5);

      if (window.onMapClickLocation) {
        window.onMapClickLocation(lat, lng);
      }
    });
  }

  invalidateSize() {
    if (this.map) {
      setTimeout(() => this.map.invalidateSize(), 150);
    }
  }

  setData(locations, roads) {
    this.locations = locations;
    this.roads = roads;
    this.render();
  }

  setHighlight(sourceId, destId, pathResult) {
    this.sourceId = sourceId ? Number(sourceId) : null;
    this.destId = destId ? Number(destId) : null;
    this.activePath = pathResult;
    this.render();
  }

  clearHighlight() {
    this.activePath = null;
    this.sourceId = null;
    this.destId = null;
    this.render();
  }

  isRoadInActivePath(u_id, v_id) {
    if (!this.activePath || !this.activePath.roads_used) return false;
    return this.activePath.roads_used.some(r =>
      (r.from_id === u_id && r.to_id === v_id) ||
      (r.from_id === v_id && r.to_id === u_id)
    );
  }

  createPinIcon(color, text) {
    return L.divIcon({
      className: 'custom-map-pin',
      html: `
        <div style="
          background: ${color};
          color: white;
          width: 30px;
          height: 30px;
          border-radius: 50% 50% 50% 0;
          transform: rotate(-45deg);
          display: flex;
          align-items: center;
          justify-content: center;
          box-shadow: 0 3px 8px rgba(0,0,0,0.3);
          border: 2px solid white;
          margin-top: -30px;
          margin-left: -15px;
        ">
          <span style="
            transform: rotate(45deg);
            font-size: 11px;
            font-weight: 700;
          ">${text}</span>
        </div>
      `,
      iconSize: [30, 30],
      iconAnchor: [15, 30]
    });
  }

  render() {
    if (!this.map) return;

    // Clear previous elements
    this.markers.forEach(m => this.map.removeLayer(m));
    this.markers.clear();

    this.roadPolylines.forEach(p => this.map.removeLayer(p));
    this.roadPolylines = [];

    if (this.pathPolyline) {
      this.map.removeLayer(this.pathPolyline);
      this.pathPolyline = null;
    }

    if (!this.locations || this.locations.length === 0) return;

    const locMap = new Map();
    const bounds = L.latLngBounds();

    // 1. Plot Location Markers
    this.locations.forEach(loc => {
      // In our storage, x = Longitude, y = Latitude
      const lat = Number(loc.y);
      const lng = Number(loc.x);

      if (isNaN(lat) || isNaN(lng)) return;

      locMap.set(loc.id, { ...loc, lat, lng });
      bounds.extend([lat, lng]);

      const isSrc = loc.id === this.sourceId;
      const isDest = loc.id === this.destId;
      const isInPath = this.activePath && this.activePath.path &&
                       this.activePath.path.some(p => p.id === loc.id);

      let pinColor = '#64748b'; // Slate neutral
      if (isSrc) pinColor = '#10b981'; // Emerald Start
      else if (isDest) pinColor = '#ef4444'; // Red Destination
      else if (isInPath) pinColor = '#7c3aed'; // Violet Route

      const icon = this.createPinIcon(pinColor, loc.id);
      const marker = L.marker([lat, lng], { icon }).addTo(this.map);

      // Popup with location details and quick action buttons
      marker.bindPopup(`
        <div style="min-width: 170px; font-family: inherit;">
          <div style="font-weight: 700; font-size: 0.95rem; color: #0f172a; margin-bottom: 2px;">
            ${loc.name}
          </div>
          <div style="font-size: 0.78rem; color: #64748b; margin-bottom: 8px;">
            Code: <code>${loc.code}</code> | ID: ${loc.id}
          </div>
          <div style="font-size: 0.75rem; color: #475569; margin-bottom: 10px;">
            GPS: ${lat.toFixed(4)}, ${lng.toFixed(4)}
          </div>
          <div style="display: flex; gap: 4px;">
            <button class="btn btn-primary btn-sm" style="flex: 1; padding: 3px 6px; font-size: 0.75rem;" onclick="window.selectAsSource(${loc.id})">Set Start</button>
            <button class="btn btn-secondary btn-sm" style="flex: 1; padding: 3px 6px; font-size: 0.75rem;" onclick="window.selectAsDest(${loc.id})">Set Dest</button>
          </div>
        </div>
      `);

      this.markers.set(loc.id, marker);
    });

    // 2. Draw Road Network Polylines
    this.roads.forEach(road => {
      const u = locMap.get(road.u_id);
      const v = locMap.get(road.v_id);
      if (!u || !v) return;

      const isPathEdge = this.isRoadInActivePath(road.u_id, road.v_id);

      // We draw inactive edges here, active edges drawn with prominence below
      if (!isPathEdge) {
        const polyline = L.polyline([[u.lat, u.lng], [v.lat, v.lng]], {
          color: '#94a3b8',
          weight: 2.5,
          opacity: 0.6,
          dashArray: road.bidirectional ? null : '4, 6'
        }).addTo(this.map);

        polyline.bindTooltip(`${road.weight} km`, {
          permanent: false,
          sticky: true,
          direction: 'center'
        });

        this.roadPolylines.push(polyline);
      }
    });

    // 3. Draw Active Dijkstra Route with Vibrant Violet Glow
    if (this.activePath && this.activePath.path && this.activePath.path.length > 1) {
      const pathCoords = this.activePath.path.map(p => {
        const item = locMap.get(p.id);
        return [item.lat, item.lng];
      });

      // Outer glow line
      const glowLine = L.polyline(pathCoords, {
        color: '#c4b5fd',
        weight: 9,
        opacity: 0.5,
        lineCap: 'round'
      }).addTo(this.map);
      this.roadPolylines.push(glowLine);

      // Core route line
      this.pathPolyline = L.polyline(pathCoords, {
        color: '#7c3aed',
        weight: 5,
        opacity: 0.95,
        lineCap: 'round',
        lineJoin: 'round'
      }).addTo(this.map);

      this.pathPolyline.bindTooltip(`Shortest Route: ${this.activePath.total_distance.toFixed(2)} km (${this.activePath.num_segments} hops)`, {
        permanent: true,
        direction: 'center',
        className: 'route-active-tooltip'
      });

      // Fit map view to route bounds
      this.map.fitBounds(this.pathPolyline.getBounds(), { padding: [50, 50] });
    } else if (bounds.isValid() && !this.activePath) {
      this.map.fitBounds(bounds, { padding: [40, 40] });
    }
  }
}
