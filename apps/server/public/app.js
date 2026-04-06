(function () {
  console.log('app.js starting');
  window.onerror = (msg, url, line) => console.log('error:', msg, 'line:', line);
  window.addEventListener('unhandledrejection', (e) =>
    console.log('unhandled rejection:', e.reason),
  );
  const API = window.location.origin;
  let logEventSource = null;
  let logPaused = false;
  let pumpPoller = null;
  let cachedPumps = [];

  function escapeHtml(str) {
    if (typeof str !== 'string') return str;
    return str.replace(
      /[&<>"']/g,
      (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' })[c],
    );
  }

  function showToast(msg, type = '') {
    const t = document.getElementById('toast');
    t.textContent = msg;
    t.className = 'toast show ' + type;
    setTimeout(() => (t.className = 'toast'), 3000);
  }

  function initNavigation() {
    const navItems = document.querySelectorAll('.nav-item[data-tool]');
    navItems.forEach((item) => {
      item.addEventListener('click', (e) => {
        e.preventDefault();
        const tool = item.dataset.tool;
        switchTool(tool);
      });
    });
  }

  function switchTool(tool) {
    document.querySelectorAll('.nav-item[data-tool]').forEach((item) => {
      item.classList.toggle('active', item.dataset.tool === tool);
    });

    document.querySelectorAll('.tool-panel').forEach((panel) => {
      panel.classList.toggle('active', panel.id === 'tool-' + tool);
    });

    if (tool === 'pump') {
      startPumpPolling();
      loadTodaysDoses();
      setTimeout(loadCommands, 100);
    } else if (tool === 'manga') {
      stopPumpPolling();
      loadMangaLibrary();
    } else {
      stopPumpPolling();
    }
  }

  function openLogViewer() {
    const viewer = document.getElementById('logViewer');
    viewer.classList.add('open');
    connectLogStream();
    loadRecentLogs();
  }

  function closeLogViewer() {
    const viewer = document.getElementById('logViewer');
    viewer.classList.remove('open');
    if (logEventSource) {
      logEventSource.close();
      logEventSource = null;
    }
  }

  function clearLogs() {
    document.getElementById('logContent').innerHTML = '<div class="log-empty">Logs cleared</div>';
  }

  function togglePauseLogs() {
    logPaused = !logPaused;
    const btn = document.getElementById('pauseLogsBtn');
    btn.textContent = logPaused ? 'Resume' : 'Pause';
  }

  async function openEspLogViewer() {
    const statusEl = document.getElementById('espStatus');
    statusEl.innerHTML = '<span class="status-checking">Checking serial connection...</span>';

    const res = await fetch(API + '/api/serial/status');
    const data = await res.json();

    if (!data.connected) {
      statusEl.innerHTML = '<span class="status-disconnected">❌ ESP32 not connected via USB</span>';
      document.getElementById('espLogContent').innerHTML = 
        '<div class="log-empty">ESP32 not connected via USB serial. Make sure the device is plugged in.</div>';
      return;
    }

    statusEl.innerHTML = '<span class="status-connected">✅ ESP32 connected</span>';
    const modal = document.getElementById('espLogModal');
    modal.classList.add('open');
    loadEspLogs();
    pollEspLogs();
  }

  function closeEspLogViewer() {
    const modal = document.getElementById('espLogModal');
    modal.classList.remove('open');
    if (espLogPollInterval) {
      clearInterval(espLogPollInterval);
      espLogPollInterval = null;
    }
  }

  async function loadEspLogs() {
    try {
      const res = await fetch(API + '/api/serial/logs');
      const data = await res.json();
      const container = document.getElementById('espLogContent');

      if (!data.connected) {
        container.innerHTML = '<div class="log-empty">Serial connection lost</div>';
        return;
      }

      const logs = data.logs || [];
      if (logs.length === 0) {
        container.innerHTML = '<div class="log-empty">No ESP32 logs yet...</div>';
        return;
      }

      container.innerHTML = logs.map(log => 
        `<div class="log-entry"><span class="log-time">${new Date(log.timestamp).toLocaleTimeString()}</span> <span class="log-message">${escapeHtml(log.message)}</span></div>`
      ).join('');
      container.scrollTop = container.scrollHeight;
    } catch (e) {
      console.error('Failed to load ESP logs:', e);
    }
  }

  let espLogPollInterval = null;

  function pollEspLogs() {
    if (espLogPollInterval) clearInterval(espLogPollInterval);
    espLogPollInterval = setInterval(loadEspLogs, 2000);
  }

  async function clearEspLogs() {
    await fetch(API + '/api/serial/clear');
    document.getElementById('espLogContent').innerHTML = '<div class="log-empty">Logs cleared</div>';
  }

  async function loadRecentLogs() {
    try {
      const res = await fetch(API + '/api/logs');
      const data = await res.json();
      const logs = data.data || [];
      const container = document.getElementById('logContent');

      if (logs.length === 0) {
        container.innerHTML = '<div class="log-empty">No logs yet...</div>';
        return;
      }

      container.innerHTML = logs.map(formatLogEntry).join('');
      container.scrollTop = container.scrollHeight;
    } catch (e) {
      console.error('Failed to load recent logs:', e);
    }
  }

  function connectLogStream() {
    if (logEventSource) {
      logEventSource.close();
    }

    logEventSource = new EventSource(API + '/api/logs/stream');

    logEventSource.onmessage = (event) => {
      if (logPaused) return;

      try {
        const entry = JSON.parse(event.data);
        const container = document.getElementById('logContent');

        const emptyEl = container.querySelector('.log-empty');
        if (emptyEl) emptyEl.remove();

        const div = document.createElement('div');
        div.innerHTML = formatLogEntry(entry);
        container.appendChild(div.firstElementChild);

        container.scrollTop = container.scrollHeight;
      } catch (e) {
        console.error('Failed to parse log:', e);
      }
    };

    logEventSource.onerror = () => {
      console.log('Log stream disconnected, reconnecting...');
    };
  }

  function formatLogEntry(entry) {
    const time = new Date(entry.timestamp).toLocaleTimeString();
    const source = entry.source ? `[${entry.source}]` : '';
    return `<div class="log-entry ${entry.level}">
      <span class="log-timestamp">${time}</span>
      <span class="log-source">${source}</span>
      <span class="log-message">${escapeHtml(entry.message)}</span>
    </div>`;
  }

  async function loadMangaLibrary() {
    const container = document.getElementById('mangaLibrary');
    const totalEl = document.getElementById('totalManga');
    const bookmarkedEl = document.getElementById('bookmarkedManga');
    const newChaptersEl = document.getElementById('newChapters');

    container.innerHTML = '<div class="loading">Loading manga library...</div>';

    try {
      const res = await fetch(API + '/api/mangas?limit=100');
      const data = await res.json();
      const mangas = data.data || [];

      totalEl.textContent = mangas.length;
      bookmarkedEl.textContent = mangas.filter((m) => m.isBookmarked).length;
      newChaptersEl.textContent = 0;

      if (mangas.length === 0) {
        container.innerHTML =
          '<div class="loading">No manga in library. Add some via the API or OpenCode.</div>';
        return;
      }

      container.innerHTML = mangas
        .map(
          (m) => `
        <div class="manga-item" data-id="${escapeHtml(m._id)}" onclick="window.openMangaDetail('${escapeHtml(m._id)}')">
          ${m.coverImage ? `<img src="${escapeHtml(m.coverImage)}" alt="${escapeHtml(m.title)}" class="manga-cover" />` : '<div class="manga-cover placeholder">📖</div>'}
          <div class="manga-info">
            <div class="manga-title">${escapeHtml(m.title)}</div>
            <div class="manga-meta">
              <span class="manga-source">${escapeHtml(m.source || 'asurascans')}</span>
              ${m.isBookmarked ? '<span class="manga-bookmarked">★</span>' : ''}
            </div>
          </div>
        </div>
      `,
        )
        .join('');
    } catch (e) {
      container.innerHTML = '<div class="loading">Error loading manga library</div>';
      showToast('Failed to load manga library', 'error');
    }
  }

  window.openMangaDetail = async function (mangaId) {
    const modal = document.getElementById('mangaModal');
    const titleEl = document.getElementById('mangaDetailTitle');
    const coverEl = document.getElementById('mangaDetailCover');
    const sourceEl = document.getElementById('mangaDetailSource');
    const chaptersEl = document.getElementById('mangaDetailChapters');
    const bookmarkedEl = document.getElementById('mangaDetailBookmarked');
    const episodesList = document.getElementById('mangaEpisodesList');

    modal.classList.add('open');
    titleEl.textContent = 'Loading...';
    coverEl.src = '';
    sourceEl.textContent = 'Source: ...';
    chaptersEl.textContent = '0 chapters';
    bookmarkedEl.style.display = 'none';
    episodesList.innerHTML = '<div class="loading">Loading episodes...</div>';

    try {
      const res = await fetch(API + '/api/mangas/' + mangaId);
      const manga = await res.json();

      titleEl.textContent = manga.title || 'Unknown';
      coverEl.src = manga.coverImage || '';
      coverEl.alt = manga.title || '';
      sourceEl.textContent = 'Source: ' + (manga.source || 'asurascans');
      chaptersEl.textContent = (manga.episodes?.length || 0) + ' chapters';
      bookmarkedEl.style.display = manga.isBookmarked ? 'inline' : 'none';

      if (manga.episodes && manga.episodes.length > 0) {
        episodesList.innerHTML = manga.episodes
          .slice(0, 50)
          .map(
            (ep) => `
          <div class="episode-item ${ep.isNew ? 'new' : ''}" onclick="window.openEpisode('${escapeHtml(ep._id)}')">
            <div>
              <span class="episode-number">Ch. ${ep.number || '?'}</span>
              <span class="episode-title">${escapeHtml(ep.title || '')}</span>
            </div>
            <span class="episode-date">${ep.crawledAt ? new Date(ep.crawledAt).toLocaleDateString() : ''}</span>
          </div>
        `,
          )
          .join('');
      } else {
        episodesList.innerHTML = '<div class="loading">No episodes yet</div>';
      }
    } catch (e) {
      episodesList.innerHTML = '<div class="loading">Error loading episodes</div>';
      showToast('Failed to load manga details', 'error');
    }
  };

  window.openEpisode = function (episodeId) {
    showToast('Episode viewer coming soon!', 'info');
  };

  function closeMangaModal() {
    document.getElementById('mangaModal').classList.remove('open');
  }

  window.syncBookmarks = async function () {
    openLogViewer();
    showToast('Syncing bookmarks - check logs below', 'info');
    try {
      await fetch(API + '/api/crawl/bookmarks/sync', { method: 'POST' });
    } catch (e) {
      console.error('Sync error:', e);
    }
  };

  window.checkNewChapters = async function () {
    openLogViewer();
    showToast('Checking new chapters - check logs below', 'info');
    try {
      await fetch(API + '/api/crawl/chapters/check');
    } catch (e) {
      console.error('Check error:', e);
    }
  };

  async function init() {
    initNavigation();

    document.getElementById('newPumpBtn')?.addEventListener('click', () => openPumpModal());
    document.getElementById('syncBtn')?.addEventListener('click', () => loadPumps());
    document.getElementById('refreshCommandsBtn')?.addEventListener('click', () => loadCommands());
    document.getElementById('doseForm')?.addEventListener('submit', submitDose);
    document.getElementById('pumpForm')?.addEventListener('submit', submitPump);
    document.getElementById('cancelPumpBtn')?.addEventListener('click', closePumpModal);
    document.querySelector('#pumpModal .modal-backdrop')?.addEventListener('click', closePumpModal);

    document.addEventListener('click', (e) => {
      const target = e.target;
      if (target.tagName === 'BUTTON') {
        const text = target.textContent.trim();
        const card = target.closest('.pump-card');
        if (card && text === 'Calibrate') {
          const pumpId = card.dataset.id;
          if (pumpId) window.calibratePump(pumpId);
        }
      }
    });

    document.getElementById('syncBookmarksBtn')?.addEventListener('click', window.syncBookmarks);
    document
      .getElementById('checkNewChaptersBtn')
      ?.addEventListener('click', window.checkNewChapters);
    document.getElementById('openLogsBtn')?.addEventListener('click', openLogViewer);
  document.getElementById('openEspLogsBtn')?.addEventListener('click', openEspLogViewer);
  document.getElementById('closeEspLogBtn')?.addEventListener('click', closeEspLogViewer);
  document.getElementById('clearEspLogsBtn')?.addEventListener('click', clearEspLogs);
    document.getElementById('closeLogsBtn')?.addEventListener('click', closeLogViewer);
    document.getElementById('clearLogsBtn')?.addEventListener('click', clearLogs);
    document.getElementById('pauseLogsBtn')?.addEventListener('click', togglePauseLogs);

    document.getElementById('closeMangaModal')?.addEventListener('click', closeMangaModal);
    document
      .querySelector('#mangaModal .modal-backdrop')
      ?.addEventListener('click', closeMangaModal);

    document
      .getElementById('closeCompareBtn')
      ?.addEventListener('click', closeSettingsCompareModal);
    document
      .querySelector('#settingsCompareModal .modal-backdrop')
      ?.addEventListener('click', closeSettingsCompareModal);

    document.getElementById('useServerSettingsBtn')?.addEventListener('click', async () => {
      const pumpId = document.getElementById('comparePumpId').textContent;
      if (!pumpId) return;
      try {
        const res = await fetch(API + '/api/pump-settings/' + pumpId);
        const settings = await res.json();
        showToast('Server settings sent to device', 'success');
        closeSettingsCompareModal();
      } catch {
        showToast('Failed to sync to device', 'error');
      }
    });

    document.getElementById('useEepromSettingsBtn')?.addEventListener('click', async () => {
      const pumpId = document.getElementById('comparePumpId').textContent;
      if (!pumpId) return;
      try {
        const res = await fetch(API + '/api/pump-settings/reset-to-eeprom/' + pumpId, { method: 'POST' });
        const data = await res.json();
        showToast('Reset device to use EEPROM settings', 'success');
        closeSettingsCompareModal();
      } catch {
        showToast('Failed to reset device', 'error');
      }
    });

    document.getElementById('pumpMenuBtn')?.addEventListener('click', (e) => {
      e.stopPropagation();
      document.getElementById('pumpMenuDropdown')?.classList.toggle('open');
    });
    document.addEventListener('click', () => {
      document.getElementById('pumpMenuDropdown')?.classList.remove('open');
    });

    loadHealth();
    startPumpPolling();
    loadTodaysDoses();
    setTimeout(loadCommands, 100);
  }

  function startPumpPolling() {
    stopPumpPolling();
    loadPumps(true);
    loadCommands(true);
    pumpPoller = setInterval(() => {
      loadPumps(false);
      loadCommands(false);
    }, 5000);
  }

  function stopPumpPolling() {
    if (pumpPoller) {
      clearInterval(pumpPoller);
      pumpPoller = null;
    }
  }

  async function loadPumps(showLoading = true) {
    const grid = document.getElementById('pumpsGrid');
    if (showLoading) {
      grid.innerHTML = '<div class="loading">Loading...</div>';
    }

    try {
      console.log('loading pumps...');
      const res = await fetch(API + '/api/pump-settings');
      console.log('pumps response:', res.status);
      const pumps = await res.json();
      console.log('pumps data:', pumps);
      const arr = Array.isArray(pumps) ? pumps : pumps.data || [];
      cachedPumps = arr;

      if (arr.length === 0) {
        grid.innerHTML = '<div class="loading">No pumps. Click "+ New Pump" to add one.</div>';
        return;
      }

      const select = document.getElementById('dosePumpSelect');
      if (select) {
        select.innerHTML = '<option value="">Select pump...</option>';

        arr.forEach((p) => {
          const id = p.pumpId || p.id || 'unknown';
          if (!select.querySelector(`option[value="${id}"]`)) {
            const opt = document.createElement('option');
            opt.value = id;
            opt.textContent = id;
            select.appendChild(opt);
          }
        });
      }

      const html = arr
        .map((p) => {
          const id = p.pumpId || p.id || 'unknown';
          const enabled = p.enabled !== false;
          const online = p.online === true;
          const wifiRssi = p.wifiRssi;
          const isDosing = p.isDosing;
          const totalDosed = p.totalDosedToday || 0;
          const settingsMatch = p.settingsMatch;
          const mismatches = p.mismatches || [];
          const lastHb = p.lastHeartbeat;
          const uptime = p.uptimeSeconds;
          const freeHeap = p.freeHeap;
          const lastSync = p.lastSettingsSync;

          let statusBadge = '';
          if (online) {
            statusBadge = `<span class="pump-status online">● Online</span>`;
          } else if (lastHb) {
            const minsAgo = Math.round((Date.now() - lastHb * 1000) / 60000);
            statusBadge = `<span class="pump-status offline">● Offline (${minsAgo}m ago)</span>`;
          } else {
            statusBadge = `<span class="pump-status unknown">● Never connected</span>`;
          }

          let syncBadge = '';
          if (online && !settingsMatch && mismatches.length > 0) {
            syncBadge = `<div class="sync-warning" title="${escapeHtml(mismatches.join(', '))}">⚠ Settings mismatch</div>`;
          } else if (online && settingsMatch) {
            syncBadge = `<div class="sync-ok">✓ Settings synced</div>`;
          }

          let extraInfo = '';
          if (online) {
            extraInfo = `
              <span><strong>WiFi:</strong> ${wifiRssi ? wifiRssi + ' dBm' : 'N/A'}</span>
              <span><strong>Dosing:</strong> ${isDosing ? 'Yes' : 'No'}</span>
              <span><strong>Today:</strong> ${totalDosed}ml</span>
              ${uptime !== null ? `<span><strong>Uptime:</strong> ${formatUptime(uptime)}</span>` : ''}
              ${freeHeap !== null ? `<span><strong>Heap:</strong> ${(freeHeap / 1024).toFixed(1)}KB</span>` : ''}
            `;
          }

          return `
          <div class="pump-card ${online ? 'online' : 'offline'}" data-id="${escapeHtml(id)}">
            <div class="pump-card-header">
              <span class="pump-name">${escapeHtml(id)}</span>
              ${statusBadge}
            </div>
            ${syncBadge}
            <div class="pump-details">
              <span><strong>Daily:</strong> ${p.dailyVolume || 0}ml</span>
              <span><strong>Day:</strong> ${p.dayStartHour || 0}:00 - ${p.dayEndHour || 24}:00</span>
              <span><strong>Day %:</strong> ${p.dayPercent || 0}%</span>
              <span><strong>Steps/ml:</strong> ${p.stepsPerML || 0}</span>
              <span><strong>Profile:</strong> ${['Slow', 'Med', 'Fast'][p.activeProfile] || 'Med'}</span>
              ${extraInfo}
            </div>
              <div class="pump-actions">
                <button class="btn btn-sm btn-secondary" onclick="window.editPump('${escapeHtml(id)}')">Edit</button>
                <button class="btn btn-sm btn-secondary" onclick="window.showHistory('${escapeHtml(id)}')">History</button>
                <button class="btn btn-sm btn-secondary" onclick="window.showSettingsCompare('${escapeHtml(id)}')">Compare</button>
                <button class="btn btn-sm btn-primary" onclick="window.calibratePump('${escapeHtml(id)}')">Calibrate</button>
                <button class="btn btn-sm btn-success" onclick="window.testDosePrompt('${escapeHtml(id)}')">Test Dose</button>
                <button class="btn btn-sm btn-danger" onclick="window.deletePump('${escapeHtml(id)}')">Delete</button>
              </div>
          </div>
        `;
        })
        .join('');
      grid.innerHTML = html;
    } catch (e) {
      console.log('error in loadPumps:', e);
      grid.innerHTML = '<div class="loading">Error loading pumps</div>';
      showToast('Failed to load pumps', 'error');
    }
  }

  window.editPump = async function (id) {
    try {
      const res = await fetch(API + '/api/pump-settings/' + id);
      if (!res.ok) return openPumpModal(id);
      const p = await res.json();
      openPumpModal(p);
    } catch {
      openPumpModal(id);
    }
  };

  window.deletePump = async function (id) {
    if (!confirm('Delete pump ' + id + '?')) return;
    try {
      const res = await fetch(API + '/api/pump-settings/' + id, { method: 'DELETE' });
      if (res.ok) {
        showToast('Pump deleted', 'success');
        loadPumps();
        loadTodaysDoses();
      } else {
        showToast('Failed to delete', 'error');
      }
    } catch {
      showToast('Error deleting pump', 'error');
    }
  };

  // Test dosing with custom volume and speed
  window.testDosePrompt = async function (pumpId) {
    const volume = prompt('Enter volume (ml):', '1');
    if (!volume || isNaN(parseFloat(volume))) return;

    const speed = prompt('Enter speed (steps/sec, empty = auto ~1500-5000):', '');
    const speedNum = speed ? parseInt(speed) : 0;

    if (
      !confirm(
        `Send TEST_DOSE: ${volume}ml${speedNum > 0 ? ' @ ' + speedNum + ' steps/sec' : ' (auto speed)'} to pump ${pumpId}?`,
      )
    )
      return;

    try {
      const res = await fetch(API + `/api/pump-commands/${pumpId}/test-dose/${volume}`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ speed: speedNum }),
      });
      if (res.ok) {
        const data = await res.json();
        showToast(`Test dose queued: ${data.message}`, 'success');
        loadCommands();
      } else {
        const err = await res.json();
        showToast(err.message || 'Failed to queue test dose', 'error');
      }
    } catch (e) {
      showToast('Error sending test dose: ' + e.message, 'error');
    }
  };

  // Legacy test dose (kept for backward compatibility)
  window.testDose = async function (pumpId, volume) {
    if (!confirm(`Send TEST_DOSE command for ${volume}ml to pump ${pumpId}?`)) return;
    try {
      const res = await fetch(API + `/api/pump-commands/${pumpId}/test-dose/${volume}`, {
        method: 'POST',
      });
      if (res.ok) {
        const data = await res.json();
        showToast(`Test dose queued: ${data.message}`, 'success');
        loadCommands(); // Refresh pending commands
      } else {
        const err = await res.json();
        showToast(err.message || 'Failed to queue test dose', 'error');
      }
    } catch (e) {
      showToast('Error sending test dose: ' + e.message, 'error');
    }
  };

  let _currentRange = '1m';
  let _allEvents = [];

  window.showHistory = async function (id) {
    const tbody = document.querySelector('#historyTable tbody');
    tbody.innerHTML = '<tr><td colspan="3" class="loading">Loading...</td></tr>';
    try {
      const res = await fetch(API + '/api/dose-events/' + id);
      const data = await res.json();
      const events = data.doses || data || [];
      _allEvents = events;
      if (!events.length) {
        tbody.innerHTML = '<tr><td colspan="3" class="empty">No dose history</td></tr>';
        return;
      }
      updateHistoryView();
    } catch {
      tbody.innerHTML = '<tr><td colspan="3" class="empty">Error loading</td></tr>';
    }
  };

  function mergeDoseEvents(events) {
    const grouped = {};
    events.forEach((e) => {
      const key = `${e.timestamp}_${e.volume}`;
      if (!grouped[key]) grouped[key] = [];
      grouped[key].push(e);
    });

    const merged = [];
    for (const [, group] of Object.entries(grouped)) {
      const completed = group.find((e) => e.status === 'completed');
      const started = group.find((e) => e.status === 'started');
      const failed = group.find((e) => e.status === 'failed');

      if (completed) {
        merged.push({
          ...completed,
          combinedStatus: started ? 'started→completed' : 'completed',
          combinedClass: started ? 'completed-started' : 'completed',
        });
      } else if (failed) {
        merged.push({ ...failed, combinedStatus: started ? 'started→failed' : 'failed', combinedClass: started ? 'failed-started' : 'failed' });
      } else if (started) {
        merged.push({ ...started, combinedStatus: 'started', combinedClass: 'started' });
      }
    }

    return merged.sort((a, b) => (b.dosingTimestamp || b.timestamp || 0) - (a.dosingTimestamp || a.timestamp || 0));
  }

  function updateHistoryView() {
    const tbody = document.querySelector('#historyTable tbody');
    const now = Date.now();
    let rangeMs;
    if (_currentRange === '1d') rangeMs = 24 * 60 * 60 * 1000;
    else if (_currentRange === '1m') rangeMs = 30 * 24 * 60 * 60 * 1000;
    else if (_currentRange === '1y') rangeMs = 365 * 24 * 60 * 60 * 1000;
    else rangeMs = 30 * 24 * 60 * 60 * 1000;

    const cutoff = now - rangeMs;
    const filtered = _allEvents.filter((e) => (e.timestamp || 0) * 1000 >= cutoff);

    if (!filtered.length) {
      tbody.innerHTML = '<tr><td colspan="4" class="empty">No dose history</td></tr>';
      renderDoseChart([]);
      return;
    }

    const merged = mergeDoseEvents(filtered);

    tbody.innerHTML = merged
      .slice(0, 50)
      .map(
        (e) => `
      <tr>
        <td>${new Date((e.dosingTimestamp || e.timestamp || 0) * 1000).toLocaleString()}</td>
        <td>${e.volume || 0}ml</td>
        <td><span class="status-badge status-${e.combinedClass || e.status}">${e.combinedStatus || e.status || '-'}</span></td>
        <td>${e.dosingTimestamp ? 'auto' : '-'}</td>
      </tr>
    `,
      )
      .join('');
    renderDoseChart(merged);
  }

  // Range button handlers
  document.querySelectorAll('.history-range-buttons button').forEach((btn) => {
    btn.addEventListener('click', () => {
      document
        .querySelectorAll('.history-range-buttons button')
        .forEach((b) => b.classList.remove('active'));
      btn.classList.add('active');
      _currentRange = btn.dataset.range;
      if (_allEvents.length) updateHistoryView();
    });
  });
  document.querySelector('.history-range-buttons button[data-range="1m"]')?.classList.add('active');

  let doseChart = null;
  function renderDoseChart(events) {
    const canvas = document.getElementById('doseChart');
    if (!canvas || typeof Chart === 'undefined') return;

    try {
      const ctx = canvas.getContext('2d');
      const sorted = events.slice(0, 200).reverse();
      const dayLimit = _currentRange === '1d' ? 1 : _currentRange === '1m' ? 30 : 365;

      const dailyData = {};
      sorted.forEach((e) => {
        if (e.status !== 'completed') return;
        const date = new Date((e.timestamp || 0) * 1000).toLocaleDateString();
        dailyData[date] = (dailyData[date] || 0) + (e.volume || 0);
      });

      const labels = Object.keys(dailyData).slice(-dayLimit);
      const values = labels.map((d) => dailyData[d]);

      if (doseChart) doseChart.destroy();

      doseChart = new Chart(ctx, {
        type: 'bar',
        data: {
          labels: labels,
          datasets: [
            {
              label: 'Volume (ml)',
              data: values,
              backgroundColor: 'rgba(14, 165, 233, 0.6)',
              borderColor: 'rgba(14, 165, 233, 1)',
              borderWidth: 1,
            },
          ],
        },
        options: {
          responsive: true,
          maintainAspectRatio: false,
          plugins: {
            legend: { display: false },
          },
          scales: {
            y: { beginAtZero: true },
          },
        },
      });
    } catch (e) {
      console.error('Chart error:', e);
    }
  }

  function openPumpModal(pump = null) {
    const isEdit = pump && typeof pump === 'object';
    document.getElementById('modalTitle').textContent = isEdit ? 'Edit Pump' : 'New Pump';
    document.getElementById('pumpId').value = isEdit ? pump.pumpId || pump.id || '' : '';
    document.getElementById('pumpId').disabled = isEdit;
    document.getElementById('pumpEnabled').checked = isEdit ? pump.enabled : true;
    document.getElementById('dailyVolume').value = isEdit ? pump.dailyVolume : 30;
    document.getElementById('dayStartHour').value = isEdit ? pump.dayStartHour : 8;
    document.getElementById('dayEndHour').value = isEdit ? pump.dayEndHour : 20;
    document.getElementById('dayPercent').value = isEdit ? pump.dayPercent : 70;
    document.getElementById('stepsPerML').value = isEdit ? pump.stepsPerML : 12800;
    document.getElementById('activeProfile').value = isEdit ? pump.activeProfile : 1;
    document.getElementById('pumpModal').classList.add('open');
    window._editingPump = isEdit ? pump.pumpId || pump.id : null;
  }

  function closePumpModal() {
    document.getElementById('pumpModal').classList.remove('open');
    window._editingPump = null;
  }

  async function submitPump(e) {
    e.preventDefault();
    const data = {
      pumpId: document.getElementById('pumpId').value,
      enabled: document.getElementById('pumpEnabled').checked,
      dailyVolume: parseFloat(document.getElementById('dailyVolume').value),
      dayStartHour: parseInt(document.getElementById('dayStartHour').value),
      dayEndHour: parseInt(document.getElementById('dayEndHour').value),
      dayPercent: parseInt(document.getElementById('dayPercent').value),
      stepsPerML: parseInt(document.getElementById('stepsPerML').value),
      activeProfile: parseInt(document.getElementById('activeProfile').value),
    };

    try {
      const res = await fetch(API + '/api/pump-settings', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(data),
      });
      if (res.ok) {
        showToast('Pump saved', 'success');
        closePumpModal();
        loadPumps();
      } else {
        const err = await res.json();
        showToast(err.message || 'Failed to save', 'error');
      }
    } catch {
      showToast('Error saving pump', 'error');
    }
  }

  let _currentCalPumpId = null;
  let _currentCommandId = null;

  window.calibratePump = async function (pumpId) {
    _currentCalPumpId = pumpId;
    document.getElementById('calPumpId').textContent = pumpId;
    document.getElementById('calMeasuredML').value = '';
    document.getElementById('calResult').innerHTML = '';
    document.getElementById('calCommandStatus').innerHTML = '';

    const res = await fetch(API + '/api/pump-settings/' + pumpId);
    if (res.ok) {
      const pump = await res.json();
      document.getElementById('calCurrentSteps').textContent = pump.stepsPerML || 'N/A';
    }

    loadCalibrationHistory(pumpId);
    switchCalTab('start');
    document.getElementById('calibrateModal').classList.add('open');
  };

  window.startCalibration = async function () {
    const btn = document.getElementById('startCalBtn');
    const status = document.getElementById('calCommandStatus');

    btn.disabled = true;
    btn.textContent = 'Sending...';

    try {
      const res = await fetch(
        API + '/api/pump-commands/' + _currentCalPumpId + '/calibrate/start',
        {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
        },
      );
      const data = await res.json();
      if (data.success) {
        _currentCommandId = data.commandId;
        status.innerHTML = `<div class="cal-success">✓ 5ml calibration started!<br>Pump will run and complete.</div>`;
        setTimeout(() => switchCalTab('measure'), 1500);
      } else {
        status.innerHTML = `<div class="cal-error">✗ ${data.message}</div>`;
      }
    } catch (e) {
      status.innerHTML = `<div class="cal-error">✗ Error: ${e.message}</div>`;
    }

    btn.disabled = false;
    btn.textContent = 'Start 5ml Calibration';
  };

  window.saveCalibration = async function () {
    const measuredML = parseFloat(document.getElementById('calMeasuredML').value);
    const resultDiv = document.getElementById('calResult');
    const saveBtn = document.getElementById('saveCalBtn');
    saveBtn.disabled = true;
    saveBtn.textContent = 'Saving...';

    if (!measuredML || measuredML <= 0) {
      resultDiv.innerHTML = '<div class="cal-error">Please enter actual ml dispensed</div>';
      saveBtn.disabled = false;
      saveBtn.textContent = 'Save Calibration';
      return;
    }

    try {
      const res = await fetch(API + '/api/pump-commands/' + _currentCalPumpId + '/calibrate/save', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ measuredML }),
      });
      const data = await res.json();
      if (data.success) {
        resultDiv.innerHTML = `<div class="cal-success">✓ Saved to pump!<br><strong>${data.stepsPerML.toFixed(2)} steps/ml</strong></div>`;
        loadPumps();
        document.getElementById('calCurrentSteps').textContent = data.stepsPerML.toFixed(2);
        loadCalibrationHistory(_currentCalPumpId);
      } else {
        resultDiv.innerHTML = `<div class="cal-error">✗ ${data.message}</div>`;
      }
    } catch (e) {
      resultDiv.innerHTML = `<div class="cal-error">✗ Error: ${e.message}</div>`;
    }

    saveBtn.disabled = false;
    saveBtn.textContent = 'Save Calibration';
  };

  window.calculateCalibration = async function () {
    const steps = parseInt(document.getElementById('calStepsCompleted').value) || 200000;
    const measuredML = parseFloat(document.getElementById('calMeasuredML').value);
    const applyToPump = document.getElementById('calApplyToPump').checked;
    const resultDiv = document.getElementById('calResult');

    if (!measuredML || measuredML <= 0) {
      resultDiv.innerHTML = '<div class="cal-error">Please enter measured volume</div>';
      return;
    }

    try {
      const res = await fetch(API + '/api/pump-commands/calculate-steps', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          pumpId: _currentCalPumpId,
          steps,
          measuredML,
          applyToPump,
        }),
      });
      const data = await res.json();
      if (data.success) {
        resultDiv.innerHTML = `
          <div class="cal-success">
            ✓ Calculated: <strong>${data.stepsPerML} steps/mL</strong><br>
            <small>${data.calculation}</small><br>
            Previous: ${data.previousStepsPerML}
          </div>
        `;
        if (applyToPump) {
          loadPumps();
          document.getElementById('calCurrentSteps').textContent = data.stepsPerML;
        }
        loadCalibrationHistory(_currentCalPumpId);
      } else {
        resultDiv.innerHTML = `<div class="cal-error">✗ ${data.message}</div>`;
      }
    } catch (e) {
      resultDiv.innerHTML = `<div class="cal-error">✗ Error: ${e.message}</div>`;
    }
  };

  async function loadCalibrationHistory(pumpId) {
    const container = document.getElementById('calHistoryList');
    try {
      const res = await fetch(API + '/api/pump-commands/' + pumpId + '/calibration-history');
      const data = await res.json();
      if (data.calibrations && data.calibrations.length > 0) {
        container.innerHTML = data.calibrations
          .map(
            (c) => `
          <div class="cal-history-item">
            <span class="cal-date">${new Date(c.timestamp).toLocaleDateString()}</span>
            <span class="cal-data">${c.steps} steps / ${c.measuredML} mL = <strong>${c.stepsPerML}</strong></span>
          </div>
        `,
          )
          .join('');
      } else {
        container.innerHTML = '<p class="empty">No calibration history</p>';
      }
    } catch {
      container.innerHTML = '<p class="empty">Error loading history</p>';
    }
  }

  function switchCalTab(tab) {
    document.querySelectorAll('.cal-tab').forEach((t) => t.classList.remove('active'));
    document.querySelectorAll('.cal-tab-content').forEach((c) => c.classList.add('hidden'));
    document.querySelector(`.cal-tab[data-tab="${tab}"]`).classList.add('active');
    document
      .getElementById('calTab' + tab.charAt(0).toUpperCase() + tab.slice(1))
      .classList.remove('hidden');
  }

  document.querySelectorAll('.cal-tab').forEach((btn) => {
    btn.addEventListener('click', () => switchCalTab(btn.dataset.tab));
  });

  document.getElementById('closeCalBtn').addEventListener('click', () => {
    document.getElementById('calibrateModal').classList.remove('open');
  });

  async function submitDose(e) {
    e.preventDefault();
    const pumpId = document.getElementById('dosePumpSelect').value;
    const volume = parseFloat(document.getElementById('doseVolume').value);
    const status = document.getElementById('doseStatus').value;
    const now = Math.floor(Date.now() / 1000);

    const data = {
      pumpId,
      eventId: String(now),
      timestamp: now,
      volume,
      status,
      success: status === 'completed' ? true : status === 'failed' ? false : null,
      metadata: { totalToday: 0, remaining: 0, isAuto: false },
    };

    try {
      const res = await fetch(API + '/api/dose-events', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(data),
      });
      if (res.ok) {
        showToast('Dose logged', 'success');
        document.getElementById('doseVolume').value = '';
        loadTodaysDoses();
        if (window._selectedPump) window.showHistory(window._selectedPump);
      } else {
        showToast('Failed to log dose', 'error');
      }
    } catch {
      showToast('Error logging dose', 'error');
    }
  }

  async function loadHealth() {
    const el = document.getElementById('healthStatus');
    try {
      const res = await fetch(API + '/api/health');
      const data = await res.json();
      const dot = el.querySelector('.health-dot');
      const text = el.querySelector('.health-text');
      if (data.status === 'ok') {
        dot.className = 'health-dot ok';
        text.textContent = 'OK';
      } else {
        dot.className = 'health-dot error';
        text.textContent = 'Error';
      }
    } catch {
      el.querySelector('.health-dot').className = 'health-dot error';
      el.querySelector('.health-text').textContent = 'Offline';
    }
  }

  async function loadCommands(showLoading = true) {
    const grid = document.getElementById('commandsGrid');
    if (showLoading) {
      grid.innerHTML = '<div class="loading">Loading...</div>';
    }

    try {
      const arr = cachedPumps.length > 0 ? cachedPumps : [];
      if (arr.length === 0) {
        const res = await fetch(API + '/api/pump-settings');
        const pumps = await res.json();
        const data = Array.isArray(pumps) ? pumps : pumps.data || [];
        cachedPumps = data;
        data.forEach((p) => {
          if (!arr.find((a) => a.pumpId === p.pumpId)) arr.push(p);
        });
      }

      let allCommands = [];
      for (const pump of arr) {
        const cmdRes = await fetch(API + '/api/pump-commands/' + pump.pumpId);
        if (cmdRes.ok) {
          const data = await cmdRes.json();
          if (data.pendingCommands && data.pendingCommands.length > 0) {
            allCommands = allCommands.concat(
              data.pendingCommands.map((c) => ({
                ...c,
                pumpId: pump.pumpId,
              })),
            );
          }
        }
      }

      if (allCommands.length === 0) {
        grid.innerHTML = '<div class="empty">No pending commands</div>';
        return;
      }

      grid.innerHTML = allCommands
        .map(
          (cmd) => `
        <div class="command-card">
          <div class="command-header">
            <span class="command-pump">${escapeHtml(cmd.pumpId)}</span>
            <span class="command-status ${cmd.status}">${cmd.status}</span>
          </div>
          <div class="command-details">
            <span><strong>Command:</strong> ${cmd.command}</span>
            ${
              cmd.command === 'SAVE_CALIBRATION'
                ? `<span><strong>Steps/mL:</strong> ${cmd.payload?.stepsPerML || cmd.stepsPerML || 'N/A'}</span>`
                : `<span><strong>Steps:</strong> ${cmd.payload?.steps || cmd.steps || 'N/A'}</span>`
            }
            ${cmd.command !== 'SAVE_CALIBRATION' ? `<span><strong>Speed:</strong> ${cmd.payload?.speed || cmd.speed || 'N/A'}</span>` : ''}
            <span><strong>ID:</strong> ${cmd.commandId}</span>
          </div>
          <div class="command-actions">
            <button class="btn btn-sm btn-secondary" onclick="window.completeCommand('${cmd.pumpId}', '${cmd.commandId}', 'completed')">Complete</button>
            <button class="btn btn-sm btn-danger" onclick="window.completeCommand('${cmd.pumpId}', '${cmd.commandId}', 'failed')">Fail</button>
          </div>
        </div>
      `,
        )
        .join('');
    } catch {
      grid.innerHTML = '<div class="empty">Error loading commands</div>';
    }
  }

  window.completeCommand = async function (pumpId, commandId, status) {
    try {
      const res = await fetch(API + '/api/pump-commands/' + pumpId + '/complete', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ commandId, status }),
      });
      if (res.ok) {
        showToast('Command marked as ' + status, 'success');
        loadCommands();
      }
    } catch {
      showToast('Error updating command', 'error');
    }
  };

  function formatUptime(seconds) {
    if (!seconds) return '0s';
    const h = Math.floor(seconds / 3600);
    const m = Math.floor((seconds % 3600) / 60);
    const s = seconds % 60;
    if (h > 0) return `${h}h ${m}m`;
    if (m > 0) return `${m}m ${s}s`;
    return `${s}s`;
  }

  window.showSettingsCompare = async function (pumpId) {
    try {
      const res = await fetch(API + '/api/pump-settings');
      const pumps = await res.json();
      const pump = Array.isArray(pumps) ? pumps.find((p) => p.pumpId === pumpId) : null;
      if (!pump) {
        showToast('Pump not found', 'error');
        return;
      }
      openSettingsCompareModal(pump);
    } catch {
      showToast('Failed to load pump data', 'error');
    }
  };

  function openSettingsCompareModal(pump) {
    document.getElementById('comparePumpId').textContent = pump.pumpId;

    const serverFields = {
      enabled: pump.enabled,
      dailyVolume: pump.dailyVolume,
      dayStartHour: pump.dayStartHour,
      dayEndHour: pump.dayEndHour,
      dayPercent: pump.dayPercent,
      stepsPerML: pump.stepsPerML,
      activeProfile: pump.activeProfile,
      pausedUntil: pump.pausedUntil,
    };

    const reportedFields = pump.online
      ? {
          enabled: pump.reportedEnabled,
          dailyVolume: pump.reportedDailyVolume,
          dayStartHour: pump.reportedDayStartHour,
          dayEndHour: pump.reportedDayEndHour,
          dayPercent: pump.reportedDayPercent,
          stepsPerML: pump.reportedStepsPerML,
          activeProfile: pump.reportedActiveProfile,
          pausedUntil: pump.reportedPausedUntil,
        }
      : null;

    const fieldLabels = {
      enabled: 'Auto Dosing',
      dailyVolume: 'Daily Volume (ml)',
      dayStartHour: 'Day Start Hour',
      dayEndHour: 'Day End Hour',
      dayPercent: 'Day Percent (%)',
      stepsPerML: 'Steps/mL',
      activeProfile: 'Speed Profile',
      pausedUntil: 'Paused Until',
    };

    const profileNames = ['Slow', 'Med', 'Fast'];
    const boolNames = { true: 'ON', false: 'OFF' };

    let rows = '';
    for (const [key, label] of Object.entries(fieldLabels)) {
      const sVal = serverFields[key];
      const rVal = reportedFields ? reportedFields[key] : undefined;
      let sDisplay =
        key === 'activeProfile'
          ? (profileNames[sVal] ?? sVal)
          : key === 'enabled'
            ? (boolNames[sVal] ?? sVal)
            : sVal;
      let rDisplay =
        rVal !== undefined
          ? key === 'activeProfile'
            ? (profileNames[rVal] ?? rVal)
            : key === 'enabled'
              ? (boolNames[rVal] ?? rVal)
              : rVal
          : 'N/A';
      const match = rVal !== undefined && rVal !== null && sVal === rVal;
      const rowClass = !pump.online ? '' : match ? 'match' : 'mismatch';

      rows += `<tr class="${rowClass}">
        <td>${label}</td>
        <td>${sDisplay}</td>
        <td>${rDisplay}</td>
        <td>${!pump.online ? '—' : match ? '✓' : '✗'}</td>
      </tr>`;
    }

    document.getElementById('compareTableBody').innerHTML = rows;

    const statusEl = document.getElementById('compareStatus');
    if (!pump.online) {
      statusEl.innerHTML = '<span class="compare-offline">Device offline — cannot verify</span>';
    } else if (pump.settingsMatch) {
      statusEl.innerHTML =
        '<span class="compare-match">✓ All settings match between server and device</span>';
    } else {
      const m = (pump.mismatches || []).join('<br>');
      statusEl.innerHTML = `<span class="compare-mismatch">⚠ Settings mismatch:<br>${m}</span>`;
    }

    const lastSyncEl = document.getElementById('compareLastSync');
    lastSyncEl.textContent = pump.lastSettingsSync
      ? new Date(pump.lastSettingsSync).toLocaleString()
      : 'Never';

    document.getElementById('settingsCompareModal').classList.add('open');
  }

  function closeSettingsCompareModal() {
    document.getElementById('settingsCompareModal').classList.remove('open');
  }

  async function loadTodaysDoses() {
    const container = document.getElementById('todaysDoses');
    container.innerHTML = '<div class="loading">Loading...</div>';

    try {
      const res = await fetch(API + '/api/pump-settings');
      const pumps = await res.json();
      const arr = Array.isArray(pumps) ? pumps : pumps.data || [];

      if (!arr.length) {
        container.innerHTML = '<div class="loading">No pumps</div>';
        return;
      }

      let html = '';
      for (const p of arr) {
        const id = p.pumpId || p.id || 'unknown';
        try {
          const tres = await fetch(API + '/api/dose-events/' + id + '/today');
          const tdata = await tres.json();
          const total = tdata.totalToday || tdata.total || 0;
          html += `<div class="dose-item"><span>${escapeHtml(id)}</span><strong>${total}ml</strong></div>`;
        } catch {
          html += `<div class="dose-item"><span>${escapeHtml(id)}</span><strong>0ml</strong></div>`;
        }
      }
      container.innerHTML = html || '<div class="loading">No data</div>';
    } catch {
      container.innerHTML = '<div class="loading">Error</div>';
    }
  }

  document.addEventListener('DOMContentLoaded', init);
})();
