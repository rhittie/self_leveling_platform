// ==================== WebSocket ====================
var ws, reconnTimer;

function connect() {
    ws = new WebSocket('ws://' + location.host + '/ws');
    ws.onopen = function() {
        document.getElementById('connDot').className = 'dot green';
        termLog('[Connected]');
    };
    ws.onclose = function() {
        document.getElementById('connDot').className = 'dot red';
        reconnTimer = setTimeout(connect, 2000);
    };
    ws.onmessage = function(e) {
        try {
            var d = JSON.parse(e.data);
            if (d.t === 'status') updateDashboard(d);
            else if (d.t === 'log') termLog(d.msg);
        } catch(err) {}
    };
}

function send(obj) {
    if (ws && ws.readyState === WebSocket.OPEN) ws.send(JSON.stringify(obj));
}

// ==================== Tab Switching ====================
document.querySelectorAll('.tab').forEach(function(btn) {
    btn.addEventListener('click', function() {
        document.querySelectorAll('.tab').forEach(function(t) { t.classList.remove('active'); });
        document.querySelectorAll('.tab-content').forEach(function(c) { c.classList.remove('active'); });
        btn.classList.add('active');
        document.getElementById('tab-' + btn.dataset.tab).classList.add('active');
    });
});

// ==================== Dashboard Updates ====================
function updateDashboard(d) {
    document.getElementById('pitch').textContent = Number(d.pitch).toFixed(2);
    document.getElementById('roll').textContent = Number(d.roll).toFixed(2);

    var badge = document.getElementById('state');
    badge.textContent = d.state;
    badge.className = 'badge badge-' + d.state.toLowerCase();

    drawBubble(Number(d.pitch), Number(d.roll), d.level);
    updateBar('m1bar', 'm1val', d.m1, d.mMin, d.mMax, d.m1Lim);
    updateBar('m2bar', 'm2val', d.m2, d.mMin, d.mMax, d.m2Lim);

    // IMU data
    document.getElementById('accel').textContent = d.ax + ', ' + d.ay + ', ' + d.az + ' g';
    document.getElementById('gyro').textContent = d.gx + ', ' + d.gy + ', ' + d.gz + ' \u00B0/s';
    document.getElementById('temp').textContent = d.temp + ' \u00B0C';
    document.getElementById('cal').textContent = d.cal ? 'Yes' : 'No';
    document.getElementById('uptime').textContent = formatUptime(d.up);

    // Motor limits tab position display
    document.getElementById('m1limpos').textContent = d.m1;
    document.getElementById('m2limpos').textContent = d.m2;

    // Store latest for limits tab
    window._lastStatus = d;
}

function updateBar(barId, valId, pos, min, max, atLimit) {
    var range = max - min;
    if (range <= 0) range = 1;
    var pct = ((pos - min) / range) * 100;
    pct = Math.max(0, Math.min(100, pct));
    var bar = document.getElementById(barId);
    bar.style.width = pct + '%';
    bar.className = atLimit ? 'bar-fill at-limit' : 'bar-fill';
    document.getElementById(valId).textContent = pos;
}

function formatUptime(ms) {
    var s = Math.floor(ms / 1000);
    if (s < 60) return s + 's';
    if (s < 3600) return Math.floor(s / 60) + 'm ' + (s % 60) + 's';
    return Math.floor(s / 3600) + 'h ' + Math.floor((s % 3600) / 60) + 'm';
}

// ==================== Bubble Level ====================
var bubbleCanvas, bubbleCtx;

function drawBubble(pitch, roll, isLevel) {
    if (!bubbleCanvas) {
        bubbleCanvas = document.getElementById('bubble');
        bubbleCtx = bubbleCanvas.getContext('2d');
    }
    var c = bubbleCtx, w = 200, h = 200, cx = w / 2, cy = h / 2, r = 90;
    c.clearRect(0, 0, w, h);

    c.fillStyle = '#0a0a1a';
    c.beginPath(); c.arc(cx, cy, r + 5, 0, Math.PI * 2); c.fill();

    c.strokeStyle = '#0f3460'; c.lineWidth = 2;
    c.beginPath(); c.arc(cx, cy, r, 0, Math.PI * 2); c.stroke();

    c.strokeStyle = '#1a3a5c'; c.lineWidth = 1;
    c.beginPath();
    c.moveTo(cx - r, cy); c.lineTo(cx + r, cy);
    c.moveTo(cx, cy - r); c.lineTo(cx, cy + r);
    c.stroke();

    c.strokeStyle = '#27ae60'; c.setLineDash([3, 3]);
    c.beginPath(); c.arc(cx, cy, Math.max(r * 0.5 / 10, 4), 0, Math.PI * 2); c.stroke();
    c.setLineDash([]);

    var maxA = 10, bx = (roll / maxA) * r, by = -(pitch / maxA) * r;
    var dist = Math.sqrt(bx * bx + by * by);
    if (dist > r - 8) { var s = (r - 8) / dist; bx *= s; by *= s; }

    var mag = Math.sqrt(pitch * pitch + roll * roll);
    c.fillStyle = isLevel ? '#2ecc71' : (mag < 2 ? '#f39c12' : '#e74c3c');
    c.beginPath(); c.arc(cx + bx, cy + by, 8, 0, Math.PI * 2); c.fill();
    c.strokeStyle = '#fff'; c.lineWidth = 1; c.stroke();

    c.fillStyle = '#888';
    c.beginPath(); c.arc(cx, cy, 2, 0, Math.PI * 2); c.fill();
}

// ==================== Motor Commands ====================
function mv(motor, steps) {
    send({cmd: 'motor', id: motor, steps: steps});
}

function mvBoth(steps) {
    // M2 direction reversed
    send({cmd: 'both', m1: steps, m2: -steps});
}

// ==================== Settings ====================
function sendGains() {
    send({
        cmd: 'gains',
        kpP: parseFloat(document.getElementById('kpP').value),
        kiP: parseFloat(document.getElementById('kiP').value),
        kpR: parseFloat(document.getElementById('kpR').value),
        kiR: parseFloat(document.getElementById('kiR').value)
    });
}

function sendTol() {
    send({cmd: 'tolerance', deg: parseFloat(document.getElementById('tol').value)});
}

function sendStabTimeout() {
    send({cmd: 'stabTimeout', sec: parseFloat(document.getElementById('stabTimeout').value)});
}

// ==================== Motor Limits Tab ====================
var limitStep = 100;
var limits = { m1in: null, m1out: null, m2in: null, m2out: null };

function setStep(val, btn) {
    val = parseInt(val);
    if (isNaN(val) || val < 1) return;
    limitStep = val;
    document.querySelectorAll('.step-btn').forEach(function(b) { b.classList.remove('active'); });
    if (btn) btn.classList.add('active');
}

function mvLimit(motor, dir) {
    var steps = limitStep * dir;
    // M2 reversed
    if (motor === 2) steps = -steps;
    send({cmd: 'motor', id: motor, steps: steps});
}

function mvBothLimit(dir) {
    var steps = limitStep * dir;
    send({cmd: 'both', m1: steps, m2: -steps});
}

function zeroMotor(motor) {
    if (motor === 1) send({cmd: 'mreset1'});
    else send({cmd: 'mreset2'});
}

function setIN(motor) {
    var d = window._lastStatus;
    if (!d) return;
    var pos = motor === 1 ? d.m1 : d.m2;
    if (motor === 1) limits.m1in = pos;
    else limits.m2in = pos;
    document.getElementById('m' + motor + 'in').textContent = pos;
    updateConfig();
}

function setOUT(motor) {
    var d = window._lastStatus;
    if (!d) return;
    var pos = motor === 1 ? d.m1 : d.m2;
    if (motor === 1) limits.m1out = pos;
    else limits.m2out = pos;
    document.getElementById('m' + motor + 'out').textContent = pos;
    updateConfig();
}

function updateConfig() {
    var lines = [];
    lines.push('M1: IN=' + (limits.m1in !== null ? limits.m1in : '?') +
               '  OUT=' + (limits.m1out !== null ? limits.m1out : '?'));
    lines.push('M2: IN=' + (limits.m2in !== null ? limits.m2in : '?') +
               '  OUT=' + (limits.m2out !== null ? limits.m2out : '?'));
    lines.push('');

    var allSet = limits.m1in !== null && limits.m1out !== null &&
                 limits.m2in !== null && limits.m2out !== null;
    if (allSet) {
        var allVals = [limits.m1in, limits.m1out, limits.m2in, limits.m2out];
        var minVal = Math.min.apply(null, allVals);
        var maxVal = Math.max.apply(null, allVals);
        lines.push('// config.h values:');
        lines.push('#define MOTOR_MIN_POSITION ' + minVal);
        lines.push('#define MOTOR_MAX_POSITION ' + maxVal);
    } else {
        lines.push('// Set all 4 limits to generate config');
    }
    document.getElementById('configOutput').textContent = lines.join('\n');
}

function copyConfig() {
    var text = document.getElementById('configOutput').textContent;
    navigator.clipboard.writeText(text).then(function() {
        // Brief flash to confirm
        var el = document.getElementById('configOutput');
        el.style.borderColor = '#2ecc71';
        setTimeout(function() { el.style.borderColor = ''; }, 500);
    });
}

// ==================== Serial Terminal ====================
function termLog(msg) {
    var el = document.getElementById('termOutput');
    el.textContent += msg + '\n';
    el.scrollTop = el.scrollHeight;
}

function sendTerminal() {
    var input = document.getElementById('termInput');
    var text = input.value.trim();
    if (!text) return;
    termLog('>> ' + text);
    send({cmd: 'serial', text: text});
    input.value = '';
}

// ==================== Start ====================
connect();
