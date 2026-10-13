/**
 * @file api.js
 * @brief REST API Client communicating with the C Backend
 */

const API = {
  async getHealth() {
    try {
      const res = await fetch('/api/health');
      return await res.json();
    } catch (e) {
      console.error('Health check failed:', e);
      return { status: 'offline', message: e.message };
    }
  },

  async getData() {
    const res = await fetch('/api/data');
    if (!res.ok) throw new Error('Failed to fetch graph data from backend');
    return await res.json();
  },

  async findShortestPath(srcId, destId, trace = true) {
    const url = `/api/shortest-path?src=${encodeURIComponent(srcId)}&dest=${encodeURIComponent(destId)}&trace=${trace ? 1 : 0}`;
    const res = await fetch(url);
    if (!res.ok) throw new Error('Shortest path calculation failed');
    return await res.json();
  },

  async addLocation(id, code, name, x = 400, y = 300) {
    const res = await fetch('/api/locations', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ id, code, name, x, y })
    });
    return await res.json();
  },

  async deleteLocation(id) {
    const res = await fetch(`/api/locations/delete?id=${encodeURIComponent(id)}`, {
      method: 'POST'
    });
    return await res.json();
  },

  async addRoad(u_id, v_id, weight, bidirectional = 1) {
    const res = await fetch('/api/roads', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ u_id, v_id, weight, bidirectional })
    });
    return await res.json();
  },

  async deleteRoad(u_id, v_id) {
    const res = await fetch(`/api/roads/delete?u_id=${encodeURIComponent(u_id)}&v_id=${encodeURIComponent(v_id)}`, {
      method: 'POST'
    });
    return await res.json();
  },

  async resetDefaultNetwork() {
    const res = await fetch('/api/reset', { method: 'POST' });
    return await res.json();
  }
};
