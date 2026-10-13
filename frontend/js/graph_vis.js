/**
 * @file graph_vis.js
 * @brief Interactive Graph Visualization Engine (Canvas-based)
 *
 * Features:
 * - High-DPI Canvas rendering
 * - Zoom, pan, and fit-to-screen controls
 * - Interactive node dragging (persists live coordinates in session)
 * - Directed/bidirectional edge weight rendering
 * - Path highlighting for Dijkstra's shortest route
 */

class GraphVisualizer {
  constructor(canvasId) {
    this.canvas = document.getElementById(canvasId);
    if (!this.canvas) return;
    this.ctx = this.canvas.getContext('2d');

    this.locations = [];
    this.roads = [];
    this.activePath = null;
    this.sourceId = null;
    this.destId = null;

    // Viewport transform
    this.scale = 1.0;
    this.panX = 0;
    this.panY = 0;

    // Interaction state
    this.isDraggingNode = false;
    this.isPanning = false;
    this.draggedNode = null;
    this.dragStartX = 0;
    this.dragStartY = 0;
    this.hoveredNode = null;

    this.initCanvas();
    this.attachEvents();
  }

  initCanvas() {
    this.resize();
    window.addEventListener('resize', () => {
      this.resize();
      this.render();
    });
  }

  resize() {
    const rect = this.canvas.parentElement.getBoundingClientRect();
    const dpr = window.devicePixelRatio || 1;
    this.canvas.width = rect.width * dpr;
    this.canvas.height = rect.height * dpr;
    this.ctx.resetTransform();
    this.ctx.scale(dpr, dpr);
    this.width = rect.width;
    this.height = rect.height;
  }

  setData(locations, roads) {
    this.locations = locations.map(loc => ({
      ...loc,
      x: Number(loc.x) || 400,
      y: Number(loc.y) || 300,
      radius: 20
    }));
    this.roads = roads;
    this.autoFit();
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

  autoFit() {
    if (!this.locations || this.locations.length === 0) return;
    let minX = Infinity, maxX = -Infinity, minY = Infinity, maxY = -Infinity;
    this.locations.forEach(l => {
      if (l.x < minX) minX = l.x;
      if (l.x > maxX) maxX = l.x;
      if (l.y < minY) minY = l.y;
      if (l.y > maxY) maxY = l.y;
    });

    const padding = 70;
    const graphW = (maxX - minX) || 400;
    const graphH = (maxY - minY) || 300;

    const scaleX = (this.width - padding * 2) / graphW;
    const scaleY = (this.height - padding * 2) / graphH;
    this.scale = Math.min(Math.max(Math.min(scaleX, scaleY), 0.5), 1.6);

    const midX = (minX + maxX) / 2;
    const midY = (minY + maxY) / 2;

    this.panX = this.width / 2 - midX * this.scale;
    this.panY = this.height / 2 - midY * this.scale;
  }

  zoom(factor) {
    const centerScreenX = this.width / 2;
    const centerScreenY = this.height / 2;
    const oldScale = this.scale;
    this.scale = Math.min(Math.max(this.scale * factor, 0.4), 3.0);

    this.panX = centerScreenX - (centerScreenX - this.panX) * (this.scale / oldScale);
    this.panY = centerScreenY - (centerScreenY - this.panY) * (this.scale / oldScale);
    this.render();
  }

  resetView() {
    this.autoFit();
    this.render();
  }

  screenToWorld(sx, sy) {
    return {
      x: (sx - this.panX) / this.scale,
      y: (sy - this.panY) / this.scale
    };
  }

  getNodeAt(worldX, worldY) {
    for (let i = this.locations.length - 1; i >= 0; i--) {
      const loc = this.locations[i];
      const dx = loc.x - worldX;
      const dy = loc.y - worldY;
      if (Math.sqrt(dx * dx + dy * dy) <= loc.radius + 6) {
        return loc;
      }
    }
    return null;
  }

  attachEvents() {
    const getPos = (e) => {
      const rect = this.canvas.getBoundingClientRect();
      const clientX = e.touches ? e.touches[0].clientX : e.clientX;
      const clientY = e.touches ? e.touches[0].clientY : e.clientY;
      return {
        x: clientX - rect.left,
        y: clientY - rect.top
      };
    };

    const onDown = (e) => {
      const pos = getPos(e);
      const world = this.screenToWorld(pos.x, pos.y);
      const hitNode = this.getNodeAt(world.x, world.y);

      if (hitNode) {
        this.isDraggingNode = true;
        this.draggedNode = hitNode;
        this.dragStartX = world.x - hitNode.x;
        this.dragStartY = world.y - hitNode.y;
      } else {
        this.isPanning = true;
        this.dragStartX = pos.x - this.panX;
        this.dragStartY = pos.y - this.panY;
      }
    };

    const onMove = (e) => {
      const pos = getPos(e);
      const world = this.screenToWorld(pos.x, pos.y);

      if (this.isDraggingNode && this.draggedNode) {
        this.draggedNode.x = world.x - this.dragStartX;
        this.draggedNode.y = world.y - this.dragStartY;
        this.render();
      } else if (this.isPanning) {
        this.panX = pos.x - this.dragStartX;
        this.panY = pos.y - this.dragStartY;
        this.render();
      } else {
        const hover = this.getNodeAt(world.x, world.y);
        if (hover !== this.hoveredNode) {
          this.hoveredNode = hover;
          this.canvas.style.cursor = hover ? 'pointer' : 'grab';
          this.render();
        }
      }
    };

    const onUp = () => {
      this.isDraggingNode = false;
      this.draggedNode = null;
      this.isPanning = false;
      this.canvas.style.cursor = 'grab';
    };

    this.canvas.addEventListener('mousedown', onDown);
    window.addEventListener('mousemove', onMove);
    window.addEventListener('mouseup', onUp);

    this.canvas.addEventListener('touchstart', (e) => { e.preventDefault(); onDown(e); }, { passive: false });
    window.addEventListener('touchmove', (e) => { onMove(e); });
    window.addEventListener('touchend', onUp);

    this.canvas.addEventListener('wheel', (e) => {
      e.preventDefault();
      const zoomFactor = e.deltaY < 0 ? 1.1 : 0.9;
      this.zoom(zoomFactor);
    }, { passive: false });
  }

  isRoadInActivePath(u_id, v_id) {
    if (!this.activePath || !this.activePath.roads_used) return false;
    return this.activePath.roads_used.some(r =>
      (r.from_id === u_id && r.to_id === v_id) ||
      (r.from_id === v_id && r.to_id === u_id)
    );
  }

  render() {
    if (!this.ctx) return;
    const ctx = this.ctx;
    ctx.clearRect(0, 0, this.width, this.height);

    ctx.save();
    ctx.translate(this.panX, this.panY);
    ctx.scale(this.scale, this.scale);

    // 1. Draw Roads (Edges)
    const locMap = new Map();
    this.locations.forEach(loc => locMap.set(loc.id, loc));

    this.roads.forEach(road => {
      const u = locMap.get(road.u_id);
      const v = locMap.get(road.v_id);
      if (!u || !v) return;

      const isPathEdge = this.isRoadInActivePath(road.u_id, road.v_id);

      ctx.beginPath();
      ctx.moveTo(u.x, u.y);
      ctx.lineTo(v.x, v.y);

      if (isPathEdge) {
        ctx.strokeStyle = '#7c3aed'; // Primary Violet Highlight
        ctx.lineWidth = 5;
        ctx.setLineDash([]);
        ctx.shadowColor = 'rgba(124, 58, 237, 0.4)';
        ctx.shadowBlur = 10;
      } else {
        ctx.strokeStyle = '#cbd5e1'; // Muted Light Slate
        ctx.lineWidth = 2;
        ctx.setLineDash([]);
        ctx.shadowBlur = 0;
      }
      ctx.stroke();
      ctx.shadowBlur = 0;

      // Draw Weight Pill
      const midX = (u.x + v.x) / 2;
      const midY = (u.y + v.y) / 2;
      const weightText = `${road.weight} km`;

      ctx.font = isPathEdge ? 'bold 11px system-ui' : '10px system-ui';
      const textWidth = ctx.measureText(weightText).width;
      const pillW = textWidth + 12;
      const pillH = 18;

      ctx.fillStyle = isPathEdge ? '#ede9fe' : '#f8fafc';
      ctx.strokeStyle = isPathEdge ? '#8b5cf6' : '#e2e8f0';
      ctx.lineWidth = 1;

      ctx.beginPath();
      ctx.roundRect(midX - pillW / 2, midY - pillH / 2, pillW, pillH, 9);
      ctx.fill();
      ctx.stroke();

      ctx.fillStyle = isPathEdge ? '#5b21b6' : '#64748b';
      ctx.textAlign = 'center';
      ctx.textBaseline = 'middle';
      ctx.fillText(weightText, midX, midY);
    });

    // 2. Draw Locations (Vertices / Nodes)
    this.locations.forEach(loc => {
      const isSrc = loc.id === this.sourceId;
      const isDest = loc.id === this.destId;
      const isHovered = this.hoveredNode && this.hoveredNode.id === loc.id;
      const isInPath = this.activePath && this.activePath.path &&
                       this.activePath.path.some(p => p.id === loc.id);

      const r = loc.radius;

      // Outer glow / accent ring
      if (isSrc || isDest || isInPath) {
        ctx.beginPath();
        ctx.arc(loc.x, loc.y, r + 5, 0, Math.PI * 2);
        ctx.fillStyle = isSrc ? 'rgba(16, 185, 129, 0.25)' :
                        isDest ? 'rgba(239, 68, 68, 0.25)' : 'rgba(124, 58, 237, 0.2)';
        ctx.fill();
      }

      // Main Node Circle
      ctx.beginPath();
      ctx.arc(loc.x, loc.y, r, 0, Math.PI * 2);

      if (isSrc) {
        ctx.fillStyle = '#10b981'; // Green for Start
        ctx.strokeStyle = '#059669';
      } else if (isDest) {
        ctx.fillStyle = '#ef4444'; // Red for Destination
        ctx.strokeStyle = '#dc2626';
      } else if (isInPath) {
        ctx.fillStyle = '#7c3aed'; // Violet for Route Nodes
        ctx.strokeStyle = '#5b21b6';
      } else if (isHovered) {
        ctx.fillStyle = '#f5f3ff';
        ctx.strokeStyle = '#7c3aed';
      } else {
        ctx.fillStyle = '#ffffff';
        ctx.strokeStyle = '#94a3b8';
      }

      ctx.lineWidth = (isSrc || isDest || isInPath || isHovered) ? 3 : 2;
      ctx.fill();
      ctx.stroke();

      // Node ID / Text inside circle
      ctx.fillStyle = (isSrc || isDest || isInPath) ? '#ffffff' : '#1e293b';
      ctx.font = 'bold 11px system-ui';
      ctx.textAlign = 'center';
      ctx.textBaseline = 'middle';
      ctx.fillText(loc.id, loc.x, loc.y);

      // Node Label (Below circle)
      ctx.font = (isSrc || isDest || isInPath) ? 'bold 12px system-ui' : '500 11px system-ui';
      const labelText = loc.name;
      const labelWidth = ctx.measureText(labelText).width;

      ctx.fillStyle = 'rgba(255, 255, 255, 0.95)';
      ctx.beginPath();
      ctx.roundRect(loc.x - labelWidth / 2 - 4, loc.y + r + 4, labelWidth + 8, 16, 4);
      ctx.fill();

      ctx.fillStyle = isSrc ? '#065f46' : isDest ? '#991b1b' : (isInPath ? '#5b21b6' : '#334155');
      ctx.textBaseline = 'top';
      ctx.fillText(labelText, loc.x, loc.y + r + 5);
    });

    ctx.restore();
  }
}
