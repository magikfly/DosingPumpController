// --- Toast/Snackbar ---
function toast(msg, isErr = false) {
  let t = document.getElementById("toast");
  t.textContent = msg;
  t.className = isErr ? "toast toast-err" : "toast";
  t.style.display = "block";
  clearTimeout(t._hideTimer);
  t._hideTimer = setTimeout(() => { t.style.display = "none"; }, 2200);
}

// --- Progress Bar ---
function showProgress(show = true) {
  document.getElementById("progressBar").style.display = show ? "block" : "none";
}

function pad2(n) { return n < 10 ? "0" + n : n; }
function pumpColor(p) {
  if (!p.enabled) return "#bbb";
  if (p.remaining < 0.1 * p.container) return "#ff9800";
  return "#19c37d";
}

function pumpStateClass(p) {
  if (!p.enabled) return "disabled";
  if (p.remaining < 0.1 * p.container) return "low";
  return "";
}

async function post(url, obj) {
  showProgress(true);
  const r = await fetch(url, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(obj)
  });
  showProgress(false);
  if (!r.ok) {
    toast("Error: " + (await r.text()), true);
    throw new Error(`POST to ${url} failed: ${r.status}`);
  }
  return await r.json();
}

// --- Calibration Modal Wizard ---
function showCalibModal(pumpIdx, pumpName, onFinish) {
  let modal = document.createElement("div");
  modal.className = "modal";
  modal.style.display = "block";
  modal.innerHTML = `
    <div class="modal-content">
      <button class="close-btn" id="closeCalib">&times;</button>
      <h3>Calibration: ${pumpName}</h3>
      <div id="calibStep1">
        <label>
          1. Enter target ml for calibration:<br>
          <input type="number" id="calibTarget" min="0.1" max="1000" value="5" style="width: 80px;">
        </label>
        <button id="startCalibBtn" class="action-btn">Start</button>
      </div>
      <div id="calibStep2" style="display:none;">
        <span>Pump running... When done, measure the actual dosed ml and enter below:</span><br>
        <input type="number" id="calibActual" min="0.1" max="1000" style="width: 80px;">
        <button id="finishCalibBtn" class="action-btn">Finish</button>
      </div>
    </div>
  `;
  document.body.appendChild(modal);
  modal.querySelector("#closeCalib").onclick = () => { document.body.removeChild(modal); };
  modal.onclick = e => { if (e.target === modal) document.body.removeChild(modal); };

  modal.querySelector("#startCalibBtn").onclick = async () => {
    const ml = parseFloat(document.getElementById("calibTarget").value);
    if (ml > 0) {
      await post("/api/calibrate_start", { pump: pumpIdx, ml });
      modal.querySelector("#calibStep1").style.display = "none";
      modal.querySelector("#calibStep2").style.display = "";
    }
  };
  modal.querySelector("#finishCalibBtn").onclick = async () => {
    const target = parseFloat(document.getElementById("calibTarget").value);
    const actual = parseFloat(document.getElementById("calibActual").value);
    if (actual > 0) {
      await post("/api/calibrate_finish", { pump: pumpIdx, target, actual });
      toast("Calibration updated!");
      document.body.removeChild(modal);
      if (onFinish) onFinish();
    }
  };
}

// --- Collapsible Dose History ---
function renderDoseHistory(card, p, pumpIdx) {
  const btn = document.createElement("button");
  btn.textContent = "Show Dose History";
  btn.className = "dose-history-toggle";
  const hist = document.createElement("div");
  hist.className = "dose-history";
  btn.onclick = function () {
    hist.classList.toggle("open");
    btn.textContent = hist.classList.contains("open") ? "Hide Dose History" : "Show Dose History";
  };
  // Simulated history, replace with API if you store dose logs!
  hist.innerHTML = "<b>Last 5 doses:</b><ul style='padding-left:1em; margin:0;'>";
  (p.history || []).slice(-5).reverse().forEach(h => {
    hist.innerHTML += `<li>${h.time || "?"} — ${h.amount} ml (${h.type})</li>`;
  });
  hist.innerHTML += "</ul>";
  card.appendChild(btn);
  card.appendChild(hist);
}

// --- Dose Timeline ---
function renderDoseTimeline(timelineDiv, times, p, pumpIdx) {
  timelineDiv.innerHTML = "";
  timelineDiv.className = "dose-timeline";
  // Draw markers for each scheduled dose
  const doses = p.dosesPerDay;
  for (let j = 0; j < doses; ++j) {
    let mins = typeof times[j] === "number" ? times[j] : Math.round(j * 1440 / doses);
    let hh = Math.floor(mins / 60), mm = mins % 60;
    let left = (mins / 1440) * 100;
    // Color marker by state
    let marker = document.createElement("div");
    marker.className = "dose-timeline-marker";
    if (!p.enabled) marker.classList.add("disabled");
    if (p.remaining < 0.1 * p.container) marker.classList.add("low");
    marker.style.left = `calc(${left}% - 7px)`;
    marker.title = (p.doseLabels && p.doseLabels[j]) ? p.doseLabels[j] : `${pad2(hh)}:${pad2(mm)}`;
    marker.innerHTML = `<span class="icon">&#9201;</span>`;
    timelineDiv.appendChild(marker);
  }
}

// --- Main Dashboard Renderer ---
async function renderDashboard() {
  const pumpsDiv = document.getElementById("pumps");
  try {
    showProgress(true);
    const resp = await fetch("/pump_status");
    if (!resp.ok) throw new Error("API error: " + resp.status);
    const data = await resp.json();
    showProgress(false);

    pumpsDiv.innerHTML = "";

    for (let i = 0; i < 3; ++i) {
      const p = data[i];
      const doseAmounts = p.doseAmounts || [];
      const card = document.createElement("div");
      card.className = "pump-card " + pumpStateClass(p);
    
      // UPDATED TOGGLE ALWAYS ON TOP, OVERLAY BELOW IT
      // 1. Schedule toggle is always at the top and not blocked by overlay
      card.innerHTML += `
        <div class="schedule-toggle-wrap">
          <label class="switch">
            <input type="checkbox" id="toggleEnabled${i}" ${p.enabled ? "checked" : ""}>
            <span class="slider"></span>
          </label>
          <span class="toggle-label">${p.enabled ? "Schedule Enabled" : "Paused"}</span>
        </div>
      `;
    
      // 2. Lock overlay (if pump is disabled)
      if (!p.enabled) {
        const lock = document.createElement("div");
        lock.className = "lock-overlay";
        lock.innerHTML = `<span class="icon">&#128274;</span>`;
        card.appendChild(lock);
      }

      card.innerHTML += `
        <h2>
          <input type="text" id="pumpnameinput${i}" value="${p.name || "Pump " + (i + 1)}" style="width:110px;"/>
          <button class="pump-name-save pump-action-btn" data-pump="${i}" title="Save name">&#128190;</button>
        </h2>
        <div class="pump-status">
          <div>
            <span style="color: ${pumpColor(p)}; font-weight:bold;">&#9679;</span>
            <span>${p.remaining.toFixed(1)} / ${p.container.toFixed(1)} ml</span>
          </div>
          <div>Calib: <b>${p.calibration.toFixed(3)} ml/s</b></div>
          <div>
            Daily: <b>${p.dailyDose} ml</b> &mdash; <b>${p.dosesPerDay}x/day</b>
            ${p.remaining < 0.1 * p.container ? '<span style="color:var(--warn);font-weight:bold;">Low!</span>' : ''}
          </div>
        </div>
      `;
      // Timeline
      const timelineDiv = document.createElement("div");
      renderDoseTimeline(timelineDiv, p.doseTimes, p, i);
      card.appendChild(timelineDiv);

      // Schedule editor with dose label
      let schedDiv = document.createElement("div");
      schedDiv.innerHTML = "<b>Edit Schedule:</b><br>";
      for (let j = 0; j < p.dosesPerDay; ++j) {
        let mins = typeof p.doseTimes[j] === "number" ? p.doseTimes[j] : Math.round(j * 1440 / p.dosesPerDay);
        let hh = Math.floor(mins / 60), mm = mins % 60;
        let doseAmount =
        typeof doseAmounts[j] === "number" && doseAmounts[j] > 0
          ? doseAmounts[j]
          : (p.dailyDose / p.dosesPerDay).toFixed(2);
          schedDiv.innerHTML += `
          <div class="timewheel-wrap">
            <select class="hh-select" id="hour${i}_${j}">${[...Array(24)].map((_,k)=>`<option${hh===k?" selected":""}>${pad2(k)}</option>`).join("")}</select>
            :
            <select class="mm-select" id="min${i}_${j}">${[...Array(60)].map((_,k)=>`<option${mm===k?" selected":""}>${pad2(k)}</option>`).join("")}</select>
            <input type="number" class="dose-amt-edit" id="doseAmt${i}_${j}" min="0" step="0.01" value="${doseAmount}">
            <span style="font-size:0.97em">ml</span>
            <button class="save-tw pump-action-btn" data-pump="${i}" data-dose="${j}" title="Save">&#128190;</button>
          </div>
        `;
      }
      card.appendChild(schedDiv);

      // Bulk settings form
      card.innerHTML += `
        <form class="pump-bulk-settings" id="bulkform${i}" autocomplete="off">
          <label>Container Volume (ml):
            <input type="number" min="1" max="100000" step="0.1" value="${p.container}" id="setvol${i}">
          </label>
          <label>Daily Dose (ml):
            <input type="number" min="0" max="100000" step="0.1" value="${p.dailyDose}" id="setdose${i}">
          </label>
          <label>Number of Doses per Day:
            <input type="number" min="1" max="24" value="${p.dosesPerDay}" id="setn${i}">
          </label>
          <button type="submit" class="pump-action-btn bulk-save" data-pump="${i}">Save</button>
        </form>
        <div class="pump-actions">
          <button class="pump-action-btn calib-btn" data-pump="${i}">&#9881; Calibrate</button>
          <button class="pump-action-btn dose-btn" data-pump="${i}">&#128167; Manual Dose</button>
          <button class="pump-action-btn prime-btn" data-pump="${i}">&#9889; Prime</button>
        </div>
      `;

      pumpsDiv.appendChild(card);

      // --- Interactivity for schedule toggle ---
      const toggle = card.querySelector(`#toggleEnabled${i}`);
      toggle.onchange = async function () {
        await post("/api/set_pump_enabled", { pump: i, enabled: toggle.checked });
        return await renderDashboard();
      };

      // --- Save dose times and labels ---
      for (let j = 0; j < p.dosesPerDay; ++j) {
        card.querySelector(`.save-tw[data-pump="${i}"][data-dose="${j}"]`).onclick = async function () {
          const h = Number(card.querySelector(`#hour${i}_${j}`).value);
          const m = Number(card.querySelector(`#min${i}_${j}`).value);
          const amt = parseFloat(card.querySelector(`#doseAmt${i}_${j}`).value);
          await post("/api/set_dose_time", { pump: i, dose: j, mins: h * 60 + m });
          await post("/api/set_dose_amount", { pump: i, dose: j, amount: amt });
          return await renderDashboard();
        };
      }

      // Bulk Save
      card.querySelector(`#bulkform${i}`).onsubmit = async (e) => {
        e.preventDefault();
        const newVol = parseFloat(card.querySelector(`#setvol${i}`).value);
        const newDose = parseFloat(card.querySelector(`#setdose${i}`).value);
        const newN = parseInt(card.querySelector(`#setn${i}`).value);
        if (newVol > 0 && newDose >= 0 && newN >= 1 && newN <= 24) {
          await post("/api/set_bulk_settings", {
            pump: i,
            vol: newVol,
            dailyDose: newDose,
            dosesPerDay: newN
          });
          toast("Settings saved!");
          return await renderDashboard();
        }
      };

      // Pump name save
      card.querySelector(".pump-name-save").onclick = async () => {
        const newName = card.querySelector(`#pumpnameinput${i}`).value.trim();
        if (newName) {
          await post("/api/set_pump_name", { pump: i, name: newName });
          toast("Pump name updated!");
          return await renderDashboard();
        }
      };

      // Calibration wizard
      card.querySelector(".calib-btn").onclick = async () => {
        showCalibModal(i, p.name || `Pump ${i+1}`, renderDashboard);
      };

      // Manual dose
      card.querySelector(".dose-btn").onclick = async () => {
        const ml = prompt("Enter ml to dose manually:");
        if (ml > 0) {
          await post("/api/dose_manual", { pump: i, ml: parseFloat(ml) });
          toast("Manual dose started!");
        }
      };

      // Prime
      card.querySelector(".prime-btn").onclick = async () => {
        const ms = prompt("Enter ms to prime pump (e.g. 2000):");
        if (ms > 0) {
          await post("/api/prime", { pump: i, ms: parseInt(ms) });
          toast("Priming...");
        }
      };

      // Dose history (simulate; replace with real backend if available)
      renderDoseHistory(card, p, i);

      // Disable controls if schedule is off
      if (!p.enabled) {
        card.querySelectorAll("input,select,button").forEach(el => {
          if (el !== toggle) el.disabled = true;
        });
        card.style.opacity = 0.66;
      } else {
        card.querySelectorAll("input,select,button").forEach(el => { el.disabled = false; });
        card.style.opacity = 1;
      }
    }
  } catch (err) {
    showProgress(false);
    pumpsDiv.innerHTML = `<div style="color:red">Failed to load pump data: ${err.message}</div>`;
    toast("Failed to load: " + err.message, true);
  }
}

// --- Settings Modal Logic ---
function setupSettingsModal() {
  const btn = document.getElementById("settingsBtn");
  const modal = document.getElementById("settingsModal");
  if (!btn || !modal) return;
  const close = document.getElementById("closeSettings");
  btn.onclick = () => (modal.style.display = "block");
  close.onclick = () => (modal.style.display = "none");
  window.onclick = e => { if (e.target === modal) modal.style.display = "none"; };

  document.getElementById("settingsForm").onsubmit = async e => {
    e.preventDefault();
    const ssid = document.getElementById("ssid").value;
    const pass = document.getElementById("password").value;
    const tz = document.getElementById("timezone").value;
    await post("/api/set_wifi", { ssid, password: pass });
    await post("/api/set_timezone", { timezone: tz });
    toast("Settings saved. The device will reboot.");
    modal.style.display = "none";
  };
}

document.addEventListener("DOMContentLoaded", () => {
  renderDashboard();
  setupSettingsModal();
});
