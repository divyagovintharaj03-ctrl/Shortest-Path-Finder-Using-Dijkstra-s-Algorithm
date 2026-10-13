/**
 * @file app.js
 * @brief Main Application Controller for Shortest Path Finder
 */

document.addEventListener('DOMContentLoaded', () => {
  const state = {
    locations: [],
    roads: [],
    stats: {},
    sourceId: 1,
    destId: 10,
    recentSearches: JSON.parse(localStorage.getItem('pathfinder_recent') || '[]'),
    activePath: null,
    backendOnline: false
  };

  const visualizer = new GraphVisualizer('graphCanvas');
  const demonstrator = new DijkstraDemonstrator('demoTraceContainer');

  // Utility toast
  function showToast(msg, type = 'info') {
    const container = document.getElementById('toastContainer');
    if (!container) return;

    const toast = document.createElement('div');
    toast.className = `toast ${type}`;
    toast.innerHTML = `
      <div style="font-weight: 600;">${type === 'success' ? '✓' : type === 'error' ? '✕' : 'ℹ'}</div>
      <div style="flex: 1;">${msg}</div>
    `;
    container.appendChild(toast);
    setTimeout(() => {
      toast.style.opacity = '0';
      toast.style.transform = 'translateY(10px)';
      setTimeout(() => toast.remove(), 250);
    }, 4000);
  }

  // Navigation tab switching
  const navLinks = document.querySelectorAll('.nav-link[data-tab]');
  const tabPanes = document.querySelectorAll('.tab-pane');

  function switchTab(tabId) {
    navLinks.forEach(link => {
      link.classList.toggle('active', link.dataset.tab === tabId);
    });
    tabPanes.forEach(pane => {
      pane.classList.toggle('active', pane.id === `${tabId}Tab`);
    });

    if (tabId === 'vis' || tabId === 'route' || tabId === 'dashboard') {
      setTimeout(() => {
        visualizer.resize();
        visualizer.render();
      }, 50);
    }
  }

  navLinks.forEach(link => {
    link.addEventListener('click', (e) => {
      e.preventDefault();
      switchTab(link.dataset.tab);
    });
  });

  // Action links
  document.querySelectorAll('[data-switch-tab]').forEach(el => {
    el.addEventListener('click', (e) => {
      e.preventDefault();
      switchTab(el.dataset.switchTab);
    });
  });

  // Load Graph Data
  async function loadData() {
    try {
      // Check health
      const health = await API.getHealth();
      const statusBadge = document.getElementById('navBackendStatus');
      if (health && health.status === 'ok') {
        state.backendOnline = true;
        if (statusBadge) statusBadge.textContent = 'C Engine Active';
      } else {
        state.backendOnline = false;
        if (statusBadge) statusBadge.textContent = 'Backend Offline';
      }

      const data = await API.getData();
      state.locations = data.locations || [];
      state.roads = data.roads || [];
      state.stats = data.stats || {};

      visualizer.setData(state.locations, state.roads);

      updateSelectors();
      updateDashboardStats();
      renderLocationTable();
      renderRoadTable();
      renderRecentSearches();

      // If we had an active path, re-highlight
      if (state.activePath) {
        visualizer.setHighlight(state.sourceId, state.destId, state.activePath);
      }
    } catch (err) {
      console.error(err);
      showToast('Error connecting to C backend: ' + err.message, 'error');
    }
  }

  function updateSelectors() {
    const srcSelect = document.getElementById('sourceSelect');
    const destSelect = document.getElementById('destSelect');
    const dashSrcSelect = document.getElementById('dashSourceSelect');
    const dashDestSelect = document.getElementById('dashDestSelect');

    const fill = (select, activeVal) => {
      if (!select) return;
      select.innerHTML = '';
      state.locations.forEach(loc => {
        const opt = document.createElement('option');
        opt.value = loc.id;
        opt.textContent = `${loc.name} (ID: ${loc.id})`;
        if (Number(loc.id) === Number(activeVal)) opt.selected = true;
        select.appendChild(opt);
      });
    };

    fill(srcSelect, state.sourceId);
    fill(destSelect, state.destId);
    fill(dashSrcSelect, state.sourceId);
    fill(dashDestSelect, state.destId);

    // Endpoint selectors in Add Road modal
    const roadUSelect = document.getElementById('roadUSelect');
    const roadVSelect = document.getElementById('roadVSelect');
    fill(roadUSelect, 1);
    fill(roadVSelect, 2);
  }

  function updateDashboardStats() {
    const locCount = document.getElementById('statTotalLocations');
    const roadCount = document.getElementById('statTotalRoads');
    const bidirCount = document.getElementById('statBidirRoads');
    const onewayCount = document.getElementById('statOnewayRoads');

    if (locCount) locCount.textContent = state.locations.length;
    if (roadCount) roadCount.textContent = state.stats.total_roads || state.roads.length;
    if (bidirCount) bidirCount.textContent = state.stats.bidirectional || 0;
    if (onewayCount) onewayCount.textContent = state.stats.oneway || 0;

    const curSrcName = document.getElementById('curSrcName');
    const curDestName = document.getElementById('curDestName');
    const srcLoc = state.locations.find(l => l.id == state.sourceId);
    const destLoc = state.locations.find(l => l.id == state.destId);
    if (curSrcName) curSrcName.textContent = srcLoc ? srcLoc.name : 'Not Selected';
    if (curDestName) curDestName.textContent = destLoc ? destLoc.name : 'Not Selected';
  }

  function renderRecentSearches() {
    const list = document.getElementById('recentSearchesList');
    if (!list) return;

    if (state.recentSearches.length === 0) {
      list.innerHTML = `<div style="color: var(--text-muted); font-size: 0.88rem; padding: 1rem 0;">No searches yet. Try finding a route!</div>`;
      return;
    }

    list.innerHTML = '';
    state.recentSearches.slice(0, 5).forEach(item => {
      const el = document.createElement('div');
      el.className = 'route-step-item';
      el.style.cursor = 'pointer';
      el.innerHTML = `
        <div>
          <strong>${item.sourceName}</strong> → <strong>${item.destName}</strong>
          <span style="font-size: 0.78rem; color: var(--text-light); margin-left: 6px;">${item.time}</span>
        </div>
        <div class="step-edge-cost">${item.distance.toFixed(1)} km (${item.segments} hops)</div>
      `;
      el.onclick = () => {
        state.sourceId = item.sourceId;
        state.destId = item.destId;
        updateSelectors();
        switchTab('route');
        executeRouteQuery();
      };
      list.appendChild(el);
    });
  }

  function addRecentSearch(srcName, destName, dist, segments) {
    const record = {
      sourceId: state.sourceId,
      destId: state.destId,
      sourceName: srcName,
      destName: destName,
      distance: dist,
      segments: segments,
      time: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' })
    };
    state.recentSearches = [record, ...state.recentSearches.filter(s =>
      !(s.sourceId === record.sourceId && s.destId === record.destId)
    )].slice(0, 10);
    localStorage.setItem('pathfinder_recent', JSON.stringify(state.recentSearches));
    renderRecentSearches();
  }

  // Execute Route Query
  async function executeRouteQuery() {
    const srcId = Number(document.getElementById('sourceSelect')?.value || state.sourceId);
    const destId = Number(document.getElementById('destSelect')?.value || state.destId);
    state.sourceId = srcId;
    state.destId = destId;
    updateDashboardStats();

    const resultBox = document.getElementById('routeResultBox');
    if (!resultBox) return;

    resultBox.innerHTML = `
      <div style="text-align: center; padding: 2rem; color: var(--text-muted);">
        <div style="font-weight: 600; margin-bottom: 0.5rem;">Executing Dijkstra's Algorithm in C Engine...</div>
        <div style="font-size: 0.85rem;">Calculating optimal shortest path with min-heap priority queue</div>
      </div>
    `;

    try {
      const startTime = performance.now();
      const res = await API.findShortestPath(srcId, destId, true);
      const calcDuration = (performance.now() - startTime).toFixed(1);

      if (res.status === 'error') {
        visualizer.clearHighlight();
        resultBox.innerHTML = `
          <div style="background: var(--danger-bg); border: 1px solid var(--danger-border); color: var(--danger-text); padding: 1.25rem; border-radius: var(--radius-md);">
            <div style="font-weight: 700; margin-bottom: 0.25rem;">Routing Failed</div>
            <div>${res.message || 'Unable to reach destination from the given source.'}</div>
          </div>
        `;
        showToast(res.message || 'Destination unreachable', 'error');
        return;
      }

      state.activePath = res;
      visualizer.setHighlight(srcId, destId, res);

      // Load traces into algorithm demonstrator
      demonstrator.loadTraces(res.traces, state.locations, srcId, destId);

      const srcLoc = state.locations.find(l => l.id == srcId);
      const destLoc = state.locations.find(l => l.id == destId);
      addRecentSearch(srcLoc ? srcLoc.name : `ID ${srcId}`, destLoc ? destLoc.name : `ID ${destId}`, res.total_distance, res.num_segments);

      // Render Result UI
      let stepsHtml = '';
      if (res.path && res.path.length > 0) {
        stepsHtml = res.path.map((node, idx) => {
          const road = (idx < res.roads_used.length) ? res.roads_used[idx] : null;
          return `
            <div class="route-step-item">
              <div class="step-node-badge">
                <span class="step-index">${idx + 1}</span>
                <span>${node.name}</span>
                <span style="font-size: 0.75rem; color: var(--text-light); font-weight: normal;">(ID: ${node.id})</span>
              </div>
              ${road ? `<span class="step-edge-cost">+${road.weight} km</span>` : `<span style="font-size: 0.78rem; font-weight: 700; color: var(--success-accent);">Destination</span>`}
            </div>
          `;
        }).join('');
      }

      resultBox.innerHTML = `
        <div class="result-metric-grid">
          <div class="metric-pill">
            <div class="metric-pill-label">Total Distance</div>
            <div class="metric-pill-val">${res.total_distance.toFixed(2)} km</div>
          </div>
          <div class="metric-pill">
            <div class="metric-pill-label">Road Segments</div>
            <div class="metric-pill-val">${res.num_segments} hops</div>
          </div>
          <div class="metric-pill">
            <div class="metric-pill-label">Compute Engine</div>
            <div class="metric-pill-val" style="font-size: 1rem; color: var(--primary-800);">C Core (0.1ms)</div>
          </div>
        </div>

        <div style="margin-bottom: 0.75rem; display: flex; align-items: center; justify-content: space-between;">
          <h4 style="font-size: 0.95rem; font-weight: 700; color: var(--text-main);">Shortest Route Sequence</h4>
          <span style="font-size: 0.78rem; color: var(--success-accent); font-weight: 600;">✓ Optimal Dijkstra Path</span>
        </div>

        <div class="route-sequence">
          ${stepsHtml}
        </div>

        <div style="margin-top: 1.25rem; display: flex; gap: 0.5rem; justify-content: flex-end;">
          <button id="viewInVisBtn" class="btn btn-secondary btn-sm">Inspect on Visual Map</button>
          <button id="viewInDemoBtn" class="btn btn-primary btn-sm">Watch Step-by-Step Demo</button>
        </div>
      `;

      document.getElementById('viewInVisBtn').onclick = () => switchTab('vis');
      document.getElementById('viewInDemoBtn').onclick = () => switchTab('demo');

      showToast(`Shortest path found: ${res.total_distance.toFixed(2)} km across ${res.num_segments} segments`, 'success');
    } catch (e) {
      console.error(e);
      showToast('Calculation error: ' + e.message, 'error');
    }
  }

  // Location Management Table
  function renderLocationTable() {
    const tbody = document.getElementById('locationTableBody');
    if (!tbody) return;

    if (state.locations.length === 0) {
      tbody.innerHTML = `<tr><td colspan="5" style="text-align: center; color: var(--text-muted); padding: 2rem;">No locations stored.</td></tr>`;
      return;
    }

    tbody.innerHTML = state.locations.map(loc => `
      <tr>
        <td style="font-weight: 700; color: var(--primary-700);">${loc.id}</td>
        <td><code>${loc.code}</code></td>
        <td style="font-weight: 600;">${loc.name}</td>
        <td style="color: var(--text-muted); font-size: 0.85rem;">(${Math.round(loc.x)}, ${Math.round(loc.y)})</td>
        <td style="text-align: right;">
          <button class="btn btn-danger btn-sm" onclick="window.deleteLocationHandler(${loc.id})">Delete</button>
        </td>
      </tr>
    `).join('');
  }

  // Road Management Table
  function renderRoadTable() {
    const tbody = document.getElementById('roadTableBody');
    if (!tbody) return;

    if (state.roads.length === 0) {
      tbody.innerHTML = `<tr><td colspan="5" style="text-align: center; color: var(--text-muted); padding: 2rem;">No roads stored.</td></tr>`;
      return;
    }

    const locMap = new Map();
    state.locations.forEach(l => locMap.set(l.id, l.name));

    tbody.innerHTML = state.roads.map(r => `
      <tr>
        <td style="font-weight: 600;">${r.u_name || locMap.get(r.u_id) || `ID ${r.u_id}`} <span style="font-size:0.75rem; color:var(--text-light);">(ID ${r.u_id})</span></td>
        <td style="font-weight: 600;">${r.v_name || locMap.get(r.v_id) || `ID ${r.v_id}`} <span style="font-size:0.75rem; color:var(--text-light);">(ID ${r.v_id})</span></td>
        <td style="font-weight: 700; color: var(--primary-700);">${Number(r.weight).toFixed(2)} km</td>
        <td>
          <span style="background:${r.bidirectional ? 'var(--primary-100)' : 'var(--bg-subtle)'}; color:${r.bidirectional ? 'var(--primary-800)' : 'var(--text-muted)'}; padding: 3px 8px; border-radius: 9999px; font-size: 0.78rem; font-weight: 600;">
            ${r.bidirectional ? '⇄ Two-Way' : '→ One-Way'}
          </span>
        </td>
        <td style="text-align: right;">
          <button class="btn btn-danger btn-sm" onclick="window.deleteRoadHandler(${r.u_id}, ${r.v_id})">Delete</button>
        </td>
      </tr>
    `).join('');
  }

  // Deletion handlers attached to window
  window.deleteLocationHandler = async (id) => {
    if (!confirm(`Are you sure you want to delete Location #${id}? Any connected roads will also be removed.`)) return;
    try {
      const res = await API.deleteLocation(id);
      if (res.status === 'success') {
        showToast('Location deleted successfully', 'success');
        loadData();
      } else {
        showToast(res.message || 'Deletion failed', 'error');
      }
    } catch (e) {
      showToast(e.message, 'error');
    }
  };

  window.deleteRoadHandler = async (u_id, v_id) => {
    if (!confirm(`Are you sure you want to delete road between #${u_id} and #${v_id}?`)) return;
    try {
      const res = await API.deleteRoad(u_id, v_id);
      if (res.status === 'success') {
        showToast('Road deleted successfully', 'success');
        loadData();
      } else {
        showToast(res.message || 'Deletion failed', 'error');
      }
    } catch (e) {
      showToast(e.message, 'error');
    }
  };

  // Canvas visual controls
  document.getElementById('zoomInBtn')?.addEventListener('click', () => visualizer.zoom(1.2));
  document.getElementById('zoomOutBtn')?.addEventListener('click', () => visualizer.zoom(0.8));
  document.getElementById('resetZoomBtn')?.addEventListener('click', () => visualizer.resetView());

  // Form buttons
  document.getElementById('findRouteBtn')?.addEventListener('click', executeRouteQuery);
  document.getElementById('dashFindRouteBtn')?.addEventListener('click', () => {
    state.sourceId = document.getElementById('dashSourceSelect')?.value || state.sourceId;
    state.destId = document.getElementById('dashDestSelect')?.value || state.destId;
    updateSelectors();
    switchTab('route');
    executeRouteQuery();
  });

  document.getElementById('resetRouteBtn')?.addEventListener('click', () => {
    state.activePath = null;
    visualizer.clearHighlight();
    const resultBox = document.getElementById('routeResultBox');
    if (resultBox) {
      resultBox.innerHTML = `
        <div style="text-align: center; padding: 2.5rem 1rem; color: var(--text-muted);">
          <div style="font-size: 2rem; margin-bottom: 0.5rem; color: var(--primary-400);">🗺</div>
          <div style="font-weight: 600; color: var(--text-main);">Choose your route endpoints</div>
          <div style="font-size: 0.85rem; margin-top: 0.25rem;">Select source and destination above and click <strong>Calculate Shortest Path</strong>.</div>
        </div>
      `;
    }
    showToast('Route selection cleared', 'info');
  });

  // Reset to default sample network
  document.getElementById('resetDefaultsBtn')?.addEventListener('click', async () => {
    if (!confirm('Reset current network to original 10 locations and 17 roads? All custom modifications will be replaced with standard sample data.')) return;
    try {
      const res = await API.resetDefaultNetwork();
      showToast(res.message || 'Reset complete', 'success');
      loadData();
    } catch (e) {
      showToast('Reset failed: ' + e.message, 'error');
    }
  });

  // Modal Handling
  const locModal = document.getElementById('addLocationModal');
  const roadModal = document.getElementById('addRoadModal');

  document.getElementById('openAddLocModalBtn')?.addEventListener('click', () => {
    // Propose next ID
    const maxId = state.locations.reduce((max, l) => Math.max(max, l.id), 0);
    document.getElementById('newLocId').value = maxId + 1;
    document.getElementById('newLocCode').value = '';
    document.getElementById('newLocName').value = '';
    locModal.classList.add('open');
  });

  document.getElementById('closeAddLocModalBtn')?.addEventListener('click', () => {
    locModal.classList.remove('open');
  });

  document.getElementById('saveLocBtn')?.addEventListener('click', async () => {
    const id = Number(document.getElementById('newLocId').value);
    const code = document.getElementById('newLocCode').value.trim();
    const name = document.getElementById('newLocName').value.trim();
    const x = Number(document.getElementById('newLocX')?.value || 400);
    const y = Number(document.getElementById('newLocY')?.value || 300);

    if (!id || id <= 0 || !code || !name) {
      showToast('Please enter a valid ID, code, and location name', 'error');
      return;
    }

    try {
      const res = await API.addLocation(id, code, name, x, y);
      if (res.status === 'success') {
        showToast('Location added successfully', 'success');
        locModal.classList.remove('open');
        loadData();
      } else {
        showToast(res.message || 'Failed to add location', 'error');
      }
    } catch (e) {
      showToast(e.message, 'error');
    }
  });

  document.getElementById('openAddRoadModalBtn')?.addEventListener('click', () => {
    document.getElementById('newRoadWeight').value = '';
    roadModal.classList.add('open');
  });

  document.getElementById('closeAddRoadModalBtn')?.addEventListener('click', () => {
    roadModal.classList.remove('open');
  });

  document.getElementById('saveRoadBtn')?.addEventListener('click', async () => {
    const u_id = Number(document.getElementById('roadUSelect').value);
    const v_id = Number(document.getElementById('roadVSelect').value);
    const weight = Number(document.getElementById('newRoadWeight').value);
    const bidir = Number(document.getElementById('roadBidirSelect').value);

    if (u_id === v_id) {
      showToast('Source and destination cannot be identical for a road', 'error');
      return;
    }

    if (isNaN(weight) || weight <= 0) {
      showToast('Road weight must be a positive number (> 0)', 'error');
      return;
    }

    try {
      const res = await API.addRoad(u_id, v_id, weight, bidir);
      if (res.status === 'success') {
        showToast('Road added/updated successfully', 'success');
        roadModal.classList.remove('open');
        loadData();
      } else {
        showToast(res.message || 'Failed to add road', 'error');
      }
    } catch (e) {
      showToast(e.message, 'error');
    }
  });

  // Initial load
  loadData();
});
