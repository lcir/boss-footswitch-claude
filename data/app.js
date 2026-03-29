'use strict';

// ── State ─────────────────────────────────────────────────────────────────────
let state = {
  mode: 'normal',
  patch: 0,
  solo: false,
  ble: false,
  effects: { boost: false, mod: false, delay: false, fx: false, reverb: false }
};

const BUTTON_IDS = ['A1', 'A2', 'B1', 'B2', 'SOLO', 'MODE'];

// Map button id → sub-label shown in each mode
const NORMAL_LABELS = {
  A1: ['A1', 'Patch'],
  A2: ['A2', 'Patch'],
  B1: ['B1', 'Patch'],
  B2: ['B2', 'Patch'],
  SOLO: ['SOLO', ''],
  MODE: ['MODE', 'FX →']
};
const EFFECTS_LABELS = {
  A1:   ['BOOST',  ''],
  A2:   ['MOD',    ''],
  B1:   ['DELAY',  ''],
  B2:   ['FX',     ''],
  SOLO: ['REVERB', ''],
  MODE: ['MODE',   '← CH']
};

// ── WebSocket ─────────────────────────────────────────────────────────────────
let ws;
let wsRetryTimer;

function connectWS() {
  const proto = location.protocol === 'https:' ? 'wss' : 'ws';
  ws = new WebSocket(`${proto}://${location.host}/ws`);

  ws.onopen = () => {
    console.log('[WS] Connected');
    document.getElementById('ws-dot').className = 'dot on';
    document.getElementById('ws-label').textContent = 'Web';
    clearTimeout(wsRetryTimer);
  };

  ws.onclose = () => {
    console.log('[WS] Disconnected — retrying in 3 s');
    document.getElementById('ws-dot').className = 'dot err';
    document.getElementById('ws-label').textContent = 'Web';
    wsRetryTimer = setTimeout(connectWS, 3000);
  };

  ws.onmessage = (e) => {
    try {
      const msg = JSON.parse(e.data);
      if      (msg.type === 'state') { state = msg; render(); }
      else if (msg.type === 'log')   { appendLog(msg); }
    } catch (err) {
      console.warn('[WS] Bad message', e.data);
    }
  };
}

function sendPress(btnId) {
  if (ws && ws.readyState === WebSocket.OPEN) {
    ws.send(JSON.stringify({ type: 'press', button: btnId }));
  }
}

// ── Render ────────────────────────────────────────────────────────────────────
function render() {
  const isEffects = state.mode === 'effects';

  // Mode banner
  const banner = document.getElementById('mode-banner');
  banner.textContent = isEffects ? 'Effects Mode' : 'Channel Mode';
  banner.className   = isEffects ? 'effects' : '';

  // BLE dot
  const bleDot = document.getElementById('ble-dot');
  bleDot.className   = 'dot ' + (state.ble ? 'on' : 'err');

  const PATCH_NAMES = ['A1', 'A2', 'B1', 'B2'];

  BUTTON_IDS.forEach((id, idx) => {
    const el     = document.getElementById('sw-' + id);
    const ledEl  = el.querySelector('.led-ring');
    const labels = isEffects ? EFFECTS_LABELS[id] : NORMAL_LABELS[id];

    el.querySelector('.label').textContent = labels[0];
    el.querySelector('.sub').textContent   = labels[1];

    // Determine LED state
    let ledState = 'inactive';
    let active   = false;

    if (!state.ble) {
      ledState = 'blink';
    } else if (!isEffects) {
      if (idx <= 3) {
        // Patch buttons
        active   = (state.patch === idx);
        ledState = active ? 'active' : 'inactive';
      } else if (id === 'SOLO') {
        active   = state.solo;
        ledState = active ? 'solo' : 'inactive';
      } else if (id === 'MODE') {
        ledState = 'inactive';
      }
    } else {
      // Effects mode
      const EFFECT_KEYS = ['boost', 'mod', 'delay', 'fx', 'reverb'];
      if (id === 'MODE') {
        ledState = 'mode-eff';
      } else {
        const key = EFFECT_KEYS[idx];
        active    = state.effects[key];
        ledState  = active ? 'fx-on' : 'fx-off';
      }
    }

    el.dataset.led    = ledState;
    el.dataset.active = String(active);
  });
}

// ── Settings form ─────────────────────────────────────────────────────────────
async function loadSettings() {
  try {
    const res = await fetch('/api/config');
    const cfg = await res.json();

    document.getElementById('cfg-ble-target').value     = cfg.ble_target ?? 'KATANA 3 MIDI';
    document.getElementById('cfg-channel').value       = cfg.midi_channel ?? 1;
    document.getElementById('cfg-pc-a1').value         = cfg.pc?.A1 ?? 1;
    document.getElementById('cfg-pc-a2').value         = cfg.pc?.A2 ?? 2;
    document.getElementById('cfg-pc-b1').value         = cfg.pc?.B1 ?? 6;
    document.getElementById('cfg-pc-b2').value         = cfg.pc?.B2 ?? 7;
    document.getElementById('cfg-cc-solo').value       = cfg.cc?.solo   ?? 73;
    document.getElementById('cfg-cc-boost').value      = cfg.cc?.boost  ?? 16;
    document.getElementById('cfg-cc-mod').value        = cfg.cc?.mod    ?? 17;
    document.getElementById('cfg-cc-delay').value      = cfg.cc?.delay  ?? 19;
    document.getElementById('cfg-cc-fx').value         = cfg.cc?.fx     ?? 18;
    document.getElementById('cfg-cc-reverb').value     = cfg.cc?.reverb ?? 20;
  } catch (e) {
    console.warn('Could not load settings', e);
  }
}

async function saveSettings(e) {
  e.preventDefault();
  const body = {
    ble_target: document.getElementById('cfg-ble-target').value.trim(),
    midi_channel: parseInt(document.getElementById('cfg-channel').value),
    pc: {
      A1: parseInt(document.getElementById('cfg-pc-a1').value),
      A2: parseInt(document.getElementById('cfg-pc-a2').value),
      B1: parseInt(document.getElementById('cfg-pc-b1').value),
      B2: parseInt(document.getElementById('cfg-pc-b2').value)
    },
    cc: {
      solo:   parseInt(document.getElementById('cfg-cc-solo').value),
      boost:  parseInt(document.getElementById('cfg-cc-boost').value),
      mod:    parseInt(document.getElementById('cfg-cc-mod').value),
      delay:  parseInt(document.getElementById('cfg-cc-delay').value),
      fx:     parseInt(document.getElementById('cfg-cc-fx').value),
      reverb: parseInt(document.getElementById('cfg-cc-reverb').value)
    }
  };
  try {
    await fetch('/api/config', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(body)
    });
    document.getElementById('save-msg').textContent = 'Saved!';
    setTimeout(() => { document.getElementById('save-msg').textContent = ''; }, 2000);
  } catch (e) {
    document.getElementById('save-msg').textContent = 'Error saving.';
  }
}

// ── Button press feedback ─────────────────────────────────────────────────────
function attachButtonListeners() {
  BUTTON_IDS.forEach(id => {
    const el = document.getElementById('sw-' + id);
    el.addEventListener('pointerdown', () => {
      el.classList.add('pressed');
      sendPress(id);
    });
    el.addEventListener('pointerup',   () => el.classList.remove('pressed'));
    el.addEventListener('pointerleave',() => el.classList.remove('pressed'));
  });
}

// ── BLE Log ───────────────────────────────────────────────────────────────────
const LOG_LEVELS = [
  { cls: 'info', icon: '·' },   // 0 INFO
  { cls: 'ok',   icon: '✓' },   // 1 OK
  { cls: 'warn', icon: '!' },   // 2 WARN
  { cls: 'err',  icon: '✕' },   // 3 ERR
];

function fmtMs(ms) {
  const s  = Math.floor(ms / 1000);
  const m  = Math.floor(s / 60);
  const ss = String(s % 60).padStart(2, '0');
  return m > 0 ? `${m}:${ss}` : `${s}s`;
}

function appendLog(entry) {
  const el   = document.getElementById('ble-log');
  const lv   = LOG_LEVELS[entry.level] || LOG_LEVELS[0];
  const row  = document.createElement('div');
  row.className = `log-entry ${lv.cls}`;
  row.innerHTML =
    `<span class="log-ts">${fmtMs(entry.ms)}</span>` +
    `<span class="log-icon">${lv.icon}</span>` +
    `<span class="log-msg">${entry.msg}</span>`;
  el.appendChild(row);
  // Keep max 200 entries in DOM
  while (el.children.length > 200) el.removeChild(el.firstChild);
  el.scrollTop = el.scrollHeight;
}

function clearLog() {
  document.getElementById('ble-log').innerHTML = '';
}

// ── WiFi reset ────────────────────────────────────────────────────────────────
async function wifiReset() {
  const btn = document.getElementById('wifi-reset-btn');
  const msg = document.getElementById('wifi-reset-msg');
  btn.disabled = true;
  try {
    await fetch('/api/wifi/reset', { method: 'POST' });
    msg.textContent = 'Restarting… připoj se na BossFootswitch-Setup';
    msg.style.color = '#f90';
  } catch (_) {
    msg.textContent = 'Hotovo — zařízení se restartuje';
    msg.style.color = '#6f6';
  }
}

// ── Init ──────────────────────────────────────────────────────────────────────
document.addEventListener('DOMContentLoaded', () => {
  attachButtonListeners();
  document.getElementById('settings-form').addEventListener('submit', saveSettings);
  loadSettings();
  connectWS();
  render();
});
