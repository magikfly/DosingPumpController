// app.js

document.addEventListener("DOMContentLoaded", () => {
  renderDashboard();
  setupSettingsModal();
});

async function getStatus() {
  const res = await fetch("/pump_status");
  if (!res.ok) throw new Error("Failed to fetch status");
  return await res.json();
}

async function post(url, data) {
  const res = await fetch(url, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(data),
  });
  if (!res.ok) throw new Error(`POST to ${url} failed: ${res.status}`);
  return await res.json();
}

function toast(msg) {
  let t = document.createElement("div");
  t.className = "toast";
  t.innerText = msg;
  document.body.appendChild(t);
  setTimeout(() => t.remove(), 2000);
}

async function renderDashboard() {
  const main = document.getElementById("pumpControls");
  main.innerHTML = `<div class="loading">Loading…</div>`;
  let data;
  try {
    data = await getStatus();
  } catch (e) {
    main.innerHTML = `<div class="loading">Unable to load data.</div>`;
    return;
  }
  main.innerHTML = "";
  const weekDays = ['S', 'M', 'T', 'W', 'T', 'F', 'S'];
  const pumps = Object.values(data);

  pumps.forEach((p, i) => {
    const card = document.createElement("div");
    card.className = "pump-card" + (!p.enabled ? " pump-disabled" : "");

    // Enable/disable toggle -- always clickable!
    card.innerHTML += `
      <div class="enabled-row">
        <label class="switch">
          <input type="checkbox" class="enabled-toggle" data-pump="${i}" ${p.enabled ? "checked" : ""}>
          <span class="slider"></span>
        </label>
        <span style="margin-left:0.7em;font-weight:600;color:${p.enabled?"#007bff":"#888"}">
          ${p.enabled ? "Enabled" : "Disabled"}
        </span>
      </div>
    `;

    // Pump name edit
    card.innerHTML += `
      <h2>
        <input type="text" id="pumpnameinput${i}" value="${p.name ? p.name.replace(/"/g, "&quot;") : "Pump " + (i + 1)}" style="width:110px;" ${!p.enabled ? "disabled" : ""}/>
        <button class="pump-name-save pump-action-btn" data-pump="${i}" title="Save name" ${!p.enabled ? "disabled" : ""}>&#128190;</button>
      </h2>
    `;

    // Bulk settings section (inputs for daily dose, doses/day, container)
    card.innerHTML += `
      <div class="bulk-settings">
        <label>
          Daily Dose (ml):
          <input type="number" class="daily-dose" data-pump="${i}" value="${p.dailyDose ?? ""}" min="0" max="5000" step="0.1" ${!p.enabled ? "disabled" : ""}>
        </label>
        <label>
          Doses per Day:
          <input type="number" class="doses-per-day" data-pump="${i}" value="${p.dosesPerDay ?? ""}" min="1" max="24" ${!p.enabled ? "disabled" : ""}>
        </label>
        <label>
          Container Volume (ml):
          <input type="number" class="container-vol" data-pump="${i}" value="${p.container ?? ""}" min="10" max="100000" ${!p.enabled ? "disabled" : ""}>
        </label>
        <button class="save-bulk-btn pump-action-btn" data-pump="${i}" ${!p.enabled ? "disabled" : ""}>Save Settings</button>
      </div>
    `;

    // Days-of-week checkboxes
    const daysMask = p.activeDays ?? 0b01111110;
    card.innerHTML += `
      <div class="days-picker" id="daysPicker${i}">
        ${weekDays.map((d, idx) =>
          `<label>
            <input type="checkbox" class="active-day-chk" data-pump="${i}" data-day="${idx}" ${daysMask & (1 << idx) ? "checked" : ""} ${!p.enabled ? "disabled" : ""}>
            ${d}
          </label>`
        ).join("")}
      </div>
    `;

    // Status and actions
    card.innerHTML += `
      <div class="pump-status">
        <div>Remaining: <b>${p.remaining?.toFixed(1) ?? "--"}</b> / ${p.container?.toFixed(1) ?? "--"} ml</div>
        <div>Calibration: <b>${p.calibration?.toFixed(3) ?? "--"} ml/s</b></div>
        <div>Daily Dose: <b>${p.dailyDose ?? "--"} ml</b> — <b>${p.dosesPerDay ?? "--"}x/day</b></div>
      </div>
      <div class="pump-actions">
        <button class="prime-btn pump-action-btn" data-pump="${i}" ${!p.enabled ? "disabled" : ""}>Prime</button>
        <button class="manual-dose-btn pump-action-btn" data-pump="${i}" ${!p.enabled ? "disabled" : ""}>Manual Dose</button>
        <button class="calibrate-btn pump-action-btn" data-pump="${i}" ${!p.enabled ? "disabled" : ""}>Calibrate</button>
      </div>
    `;

    // Editable Schedule (with Save button)
    if (Array.isArray(p.doseTimes) && Array.isArray(p.doseAmounts)) {
      card.innerHTML += `
        <div class="schedule-list" id="schedList${i}">
          <h3>Schedule</h3>
          ${p.doseTimes.map((t, j) => {
            const hh = String(Math.floor(t / 60)).padStart(2, "0");
            const mm = String(t % 60).padStart(2, "0");
            const amt = p.doseAmounts[j] !== undefined ? p.doseAmounts[j] : 0;
            return `
              <div class="sched-row" data-pump="${i}" data-dose="${j}">
                <label>Time:
                  <select class="dose-hour" data-pump="${i}" data-dose="${j}" ${!p.enabled ? "disabled" : ""}>
                    ${[...Array(24).keys()].map(h => `<option value="${h}" ${h == parseInt(hh) ? 'selected' : ''}>${String(h).padStart(2, '0')}</option>`).join("")}
                  </select> :
                  <select class="dose-min" data-pump="${i}" data-dose="${j}" ${!p.enabled ? "disabled" : ""}>
                    ${[...Array(60).keys()].map(m => `<option value="${m}" ${m == parseInt(mm) ? 'selected' : ''}>${String(m).padStart(2, '0')}</option>`).join("")}
                  </select>
                </label>
                <label>Amount:
                  <input type="number" class="dose-amount" data-pump="${i}" data-dose="${j}" value="${amt}" min="0" max="5000" step="0.1" style="width:65px;" ${!p.enabled ? "disabled" : ""}>
                  ml
                </label>
              </div>
            `;
          }).join("")}
        </div>
        <button class="save-sched-btn pump-action-btn" data-pump="${i}" ${!p.enabled ? "disabled" : ""}>Save Schedule</button>
      `;
    }

    main.appendChild(card);
  });

  // Handler: Enabled toggle (ALWAYS ACTIVE)
  document.querySelectorAll(".enabled-toggle").forEach(chk => {
    chk.onchange = async () => {
      const pump = +chk.dataset.pump;
      await post("/api/set_pump_enabled", { pump, enabled: chk.checked });
      toast(`Pump ${chk.checked ? "enabled" : "disabled"}!`);
      await renderDashboard();
    };
  });

  // Handler: Name save
  document.querySelectorAll(".pump-name-save").forEach(btn => {
    btn.onclick = async () => {
      const i = +btn.dataset.pump;
      const name = document.getElementById(`pumpnameinput${i}`).value;
      await post("/api/set_pump_name", { pump: i, name });
      toast("Pump name saved!");
      await renderDashboard();
    };
  });

  // Handler: Bulk settings (mls per day, # doses, container vol)
  document.querySelectorAll(".save-bulk-btn").forEach(btn => {
    btn.onclick = async () => {
      const pump = +btn.dataset.pump;
      const dailyDose = parseFloat(document.querySelector(`.daily-dose[data-pump="${pump}"]`).value) || 0;
      const dosesPerDay = parseInt(document.querySelector(`.doses-per-day[data-pump="${pump}"]`).value) || 1;
      const container = parseFloat(document.querySelector(`.container-vol[data-pump="${pump}"]`).value) || 0;
      await post("/api/set_bulk_settings", { pump, dailyDose, dosesPerDay, vol: container });
      toast("Settings saved!");
      await renderDashboard();
    };
  });

  // Handler: Day-of-week checkboxes (instant save)
  document.querySelectorAll(".active-day-chk").forEach(chk => {
    chk.onchange = async () => {
      const pump = +chk.dataset.pump;
      let mask = 0;
      document.querySelectorAll(`#daysPicker${pump} .active-day-chk`).forEach((box, i) => {
        if (box.checked) mask |= (1 << i);
      });
      await post("/api/set_active_days", { pump, days: mask });
      toast("Days updated!");
      // Don't rerender for smoother UX
    };
  });

  // Handler: Prime (press and hold)
  document.querySelectorAll(".prime-btn").forEach(btn => {
    btn.onmousedown = btn.ontouchstart = async e => {
      e.preventDefault();
      const pump = +btn.dataset.pump;
      await post("/api/prime_start", { pump });
      toast("Priming…");
    };
    btn.onmouseup = btn.onmouseleave = btn.ontouchend = async e => {
      e.preventDefault();
      const pump = +btn.dataset.pump;
      await post("/api/prime_stop", { pump });
      toast("Prime stopped");
    };
  });

  // Handler: Manual dose
  document.querySelectorAll(".manual-dose-btn").forEach(btn => {
    btn.onclick = async () => {
      const pump = +btn.dataset.pump;
      let ml = prompt("Enter amount to dose (ml):");
      if (ml && ml > 0) {
        await post("/api/manual_dose", { pump, ml: parseFloat(ml) });
        toast("Manual dose started!");
      }
    };
  });

  // Handler: Calibrate
  document.querySelectorAll(".calibrate-btn").forEach(btn => {
    btn.onclick = async () => {
      const pump = +btn.dataset.pump;
      let ml = prompt("Calibration: enter target amount to pump (ml):");
      if (ml && ml > 0) {
        await post("/api/calibrate", { pump, ml: parseFloat(ml) });
        toast("Calibration started!");
      }
    };
  });

  // Handler: Save schedule
  document.querySelectorAll(".save-sched-btn").forEach(btn => {
    btn.onclick = async () => {
      const pump = +btn.dataset.pump;
      // Collect times and amounts
      const schedRows = document.querySelectorAll(`.sched-row[data-pump="${pump}"]`);
      let times = [], amounts = [];
      schedRows.forEach(row => {
        const j = row.dataset.dose;
        const h = row.querySelector(".dose-hour").value;
        const m = row.querySelector(".dose-min").value;
        times[j] = (parseInt(h) * 60) + parseInt(m);
        amounts[j] = parseFloat(row.querySelector(".dose-amount").value) || 0;
      });
      await post("/api/set_schedule", { pump, times, amounts });
      toast("Schedule saved!");
      await renderDashboard();
    };
  });
}

// --- Settings Modal logic (WiFi, timezone, etc) ---

function setupSettingsModal() {
  const settingsIcon = document.getElementById("settingsIcon");
  const modal = document.getElementById("settingsModal");
  if (!modal) return;
  // Open/close logic
  settingsIcon.onclick = () => { modal.style.display = "block"; };
  modal.querySelector("#closeSettings").onclick = () => { modal.style.display = "none"; };
  window.onclick = e => { if (e.target === modal) modal.style.display = "none"; };
  // Submit
  modal.querySelector("#settingsForm").onsubmit = async (e) => {
    e.preventDefault();
    const payload = {
      ssid: modal.querySelector("#ssid").value,
      password: modal.querySelector("#password").value,
      timezone: modal.querySelector("#timezone").value
    };
    await post("/settings", payload);
    toast("Settings saved. Device will reboot.");
    setTimeout(() => { modal.style.display = "none"; }, 1200);
  };
}
