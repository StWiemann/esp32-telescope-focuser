// ESP32 Focuser Web UI – app.js
// Vanilla JS, no framework. Fetches status every second.

const API = '';  // same origin

// ── Step sizes (loaded from /api/config, updated on save) ─────────────────
let stepSm = 10, stepMd = 100, stepLg = 1000;

// ── Toast notifications ────────────────────────────────────────────────────
let toastTimer;
function toast(msg, ms = 2500) {
  const el = document.getElementById('toast');
  el.textContent = msg;
  el.classList.add('show');
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => el.classList.remove('show'), ms);
}

// ── Step button labels and actions ─────────────────────────────────────────
function updateStepButtons() {
  document.getElementById('btnLgLeft').textContent  = `\u22d8\u22d8${stepLg}\u22d8\u22d8`;
  document.getElementById('btnMdLeft').textContent  = `\u22d8${stepMd}\u22d8`;
  document.getElementById('btnSmLeft').textContent  = `\u22d8${stepSm}`;
  document.getElementById('btnSmRight').textContent = `${stepSm}\u22d9`;
  document.getElementById('btnMdRight').textContent = `\u22d9${stepMd}\u22d9`;
  document.getElementById('btnLgRight').textContent = `\u22d9\u22d9${stepLg}\u22d9\u22d9`;
}

// Wire up step button clicks once DOM is ready
['btnLgLeft','btnMdLeft','btnSmLeft','btnSmRight','btnMdRight','btnLgRight'].forEach(id => {
  document.getElementById(id).addEventListener('click', () => {
    const delta = {
      btnLgLeft:  -stepLg,
      btnMdLeft:  -stepMd,
      btnSmLeft:  -stepSm,
      btnSmRight:  stepSm,
      btnMdRight:  stepMd,
      btnLgRight:  stepLg,
    }[id];
    moveby(delta);
  });
});

// ── Status polling ─────────────────────────────────────────────────────────
function updateStatus() {
  fetch(API + '/api/status')
    .then(r => r.json())
    .then(d => {
      const pos = document.getElementById('pos');
      pos.textContent = d.isZeroed ? d.position : '----';

      document.getElementById('tgt-line').textContent =
        'Target: ' + (d.isZeroed ? d.target : '----');

      const badge = document.getElementById('moving-badge');
      if (d.isMoving) {
        const dir = d.position < d.target ? '>>>' : '<<<';
        badge.innerHTML = '<span class="badge badge-warn moving-indicator">MOVING ' + dir + '</span>';
      } else if (!d.isZeroed) {
        badge.innerHTML = '<span class="badge badge-err">UNZEROED</span>';
      } else {
        badge.innerHTML = '<span class="badge badge-ok">READY</span>';
      }

      document.getElementById('zeroed').textContent  = d.isZeroed ? 'Yes' : 'No';
      document.getElementById('maxPos').textContent  = d.maxPosition;
      document.getElementById('fw').textContent      = d.firmware;

      const tempEl = document.getElementById('temperature');
      if (d.temperature !== null && d.temperature !== undefined) {
        tempEl.textContent = d.temperature.toFixed(1) + ' \u00b0C';
      } else if (d.tempAvailable === false) {
        tempEl.textContent = 'No sensor';
      } else {
        tempEl.textContent = '---';
      }

      const wok = d.wifi && d.wifi.ok;
      document.getElementById('wifiStatus').innerHTML =
        wok ? '<span class="badge badge-ok">CONNECTED</span>'
            : '<span class="badge badge-err">LOST</span>';
      document.getElementById('ip').textContent   = d.wifi ? d.wifi.ip   : '-';
      document.getElementById('rssi').textContent = d.wifi ? (d.wifi.rssi + ' dBm') : '-';
    })
    .catch(() => {
      document.getElementById('wifiStatus').textContent = 'unreachable';
    });
}

setInterval(updateStatus, 1000);
updateStatus();

// ── Control actions ────────────────────────────────────────────────────────
function post(url, body) {
  return fetch(API + url, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: body ? JSON.stringify(body) : undefined
  }).then(r => r.json());
}

function moveby(delta) {
  post('/api/moveby', { delta })
    .then(d => { if (!d.ok) toast('Error: ' + d.error); })
    .catch(() => toast('Request failed'));
}

function moveTo() {
  const pos = parseInt(document.getElementById('moveTarget').value, 10);
  if (isNaN(pos)) { toast('Enter a position first'); return; }
  post('/api/move', { position: pos })
    .then(d => { if (!d.ok) toast('Error: ' + d.error); else toast('Moving to ' + pos); })
    .catch(() => toast('Request failed'));
}

function halt() {
  post('/api/halt')
    .then(() => toast('Stopped'))
    .catch(() => toast('Request failed'));
}

function doZero() {
  if (!confirm('Set current position as ZERO?\n\nMake sure the focuser is fully retracted.'))
    return;
  post('/api/zero')
    .then(d => {
      if (d.ok) toast('Position zeroed');
      else      toast('Error: ' + d.error);
    })
    .catch(() => toast('Request failed'));
}

// ── Config load / save ─────────────────────────────────────────────────────
function loadConfig() {
  fetch(API + '/api/config')
    .then(r => r.json())
    .then(cfg => {
      document.getElementById('cfgName').value        = cfg.focuserName    || '';
      document.getElementById('cfgMaxPos').value      = cfg.maxPosition    || 0;
      document.getElementById('cfgBacklash').value    = cfg.backlashSteps  || 0;
      document.getElementById('cfgBacklashDir').value = cfg.backlashApproachDir || 1;
      document.getElementById('cfgManStep').value     = cfg.manualStepSize || 10;
      document.getElementById('cfgStepMd').value      = cfg.stepMedium     || 100;
      document.getElementById('cfgStepLg').value      = cfg.stepLarge      || 1000;
      document.getElementById('cfgManSpeed').value    = cfg.manualSpeed    || 400;
      document.getElementById('cfgMaxSpeed').value    = cfg.motorMaxSpeed  || 500;
      document.getElementById('cfgReversed').value    = String(cfg.motorReversed || false);
      document.getElementById('cfgStepsRev').value    = cfg.stepsPerRev    || 4076;
      document.getElementById('cfgMotorTeeth').value  = cfg.motorPulleyTeeth || 16;
      document.getElementById('cfgFocTeeth').value    = cfg.focuserPulleyTeeth || 60;

      stepSm = cfg.manualStepSize || 10;
      stepMd = cfg.stepMedium     || 100;
      stepLg = cfg.stepLarge      || 1000;
      updateStepButtons();
      updateRatioDisplay(cfg);
    })
    .catch(() => toast('Could not load config'));
}

function updateRatioDisplay(cfg) {
  const m = parseFloat(document.getElementById('cfgMotorTeeth').value) || cfg.motorPulleyTeeth || 16;
  const f = parseFloat(document.getElementById('cfgFocTeeth').value)   || cfg.focuserPulleyTeeth || 60;
  const r = (f / m).toFixed(3);
  document.getElementById('ratioDisplay').textContent = 'Belt ratio: ' + f + '/' + m + ' = ' + r + ':1';
}

['cfgMotorTeeth','cfgFocTeeth'].forEach(id => {
  document.getElementById(id).addEventListener('input', () => updateRatioDisplay({}));
});

function saveConfig() {
  const body = {
    focuserName:         document.getElementById('cfgName').value,
    maxPosition:         parseInt(document.getElementById('cfgMaxPos').value, 10),
    backlashSteps:       parseInt(document.getElementById('cfgBacklash').value, 10),
    backlashApproachDir: parseInt(document.getElementById('cfgBacklashDir').value, 10),
    manualStepSize:      parseInt(document.getElementById('cfgManStep').value, 10),
    stepMedium:          parseInt(document.getElementById('cfgStepMd').value, 10),
    stepLarge:           parseInt(document.getElementById('cfgStepLg').value, 10),
    manualSpeed:         parseFloat(document.getElementById('cfgManSpeed').value),
    motorMaxSpeed:       parseFloat(document.getElementById('cfgMaxSpeed').value),
    motorReversed:       document.getElementById('cfgReversed').value === 'true',
    stepsPerRev:         parseInt(document.getElementById('cfgStepsRev').value, 10),
    motorPulleyTeeth:    parseInt(document.getElementById('cfgMotorTeeth').value, 10),
    focuserPulleyTeeth:  parseInt(document.getElementById('cfgFocTeeth').value, 10),
  };

  post('/api/config', body)
    .then(d => {
      if (d.ok) {
        // Update live step sizes so buttons reflect new values immediately
        stepSm = body.manualStepSize;
        stepMd = body.stepMedium;
        stepLg = body.stepLarge;
        updateStepButtons();

        document.getElementById('configSaved').style.display = 'inline';
        setTimeout(() => { document.getElementById('configSaved').style.display = 'none'; }, 3000);
        toast('Configuration saved');
      } else {
        toast('Error: ' + d.error);
      }
    })
    .catch(() => toast('Request failed'));
}

// Load config on page load
loadConfig();
