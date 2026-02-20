// WebSocket connection
let ws;
let reconnectTimer;

function connect() {
    ws = new WebSocket('ws://' + location.host + '/ws');
    ws.onopen = function() {
        document.getElementById('connDot').className = 'dot green';
    };
    ws.onclose = function() {
        document.getElementById('connDot').className = 'dot red';
        reconnectTimer = setTimeout(connect, 2000);
    };
    ws.onmessage = function(e) {
        try {
            var d = JSON.parse(e.data);
            updateDashboard(d);
        } catch(err) {}
    };
}

function updateDashboard(d) {
    // Angles
    document.getElementById('pitch').textContent = Number(d.pitch).toFixed(2);
    document.getElementById('roll').textContent = Number(d.roll).toFixed(2);

    // State badge
    var badge = document.getElementById('state');
    badge.textContent = d.state;
    badge.className = 'badge badge-' + d.state.toLowerCase();

    // Bubble level
    drawBubble(Number(d.pitch), Number(d.roll), d.level);

    // Motor bars
    updateBar('m1bar', 'm1val', d.m1, d.mMin, d.mMax, d.m1Lim);
    updateBar('m2bar', 'm2val', d.m2, d.mMin, d.mMax, d.m2Lim);
}

function updateBar(barId, valId, pos, min, max, atLimit) {
    var range = max - min;
    if (range <= 0) range = 1;
    var pct = ((pos - min) / range) * 100;
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    var bar = document.getElementById(barId);
    bar.style.width = pct + '%';
    bar.className = atLimit ? 'bar-fill at-limit' : 'bar-fill';
    document.getElementById(valId).textContent = pos;
}

// Bubble level canvas
var bubbleCanvas = null;
var bubbleCtx = null;

function drawBubble(pitch, roll, isLevel) {
    if (!bubbleCanvas) {
        bubbleCanvas = document.getElementById('bubble');
        bubbleCtx = bubbleCanvas.getContext('2d');
    }
    var c = bubbleCtx;
    var w = 200, h = 200;
    var cx = w / 2, cy = h / 2;
    var r = 90;

    c.clearRect(0, 0, w, h);

    // Background
    c.fillStyle = '#0a0a1a';
    c.beginPath();
    c.arc(cx, cy, r + 5, 0, Math.PI * 2);
    c.fill();

    // Outer ring
    c.strokeStyle = '#0f3460';
    c.lineWidth = 2;
    c.beginPath();
    c.arc(cx, cy, r, 0, Math.PI * 2);
    c.stroke();

    // Crosshairs
    c.strokeStyle = '#1a3a5c';
    c.lineWidth = 1;
    c.beginPath();
    c.moveTo(cx - r, cy); c.lineTo(cx + r, cy);
    c.moveTo(cx, cy - r); c.lineTo(cx, cy + r);
    c.stroke();

    // Tolerance circle
    var tolR = Math.max(r * 0.5 / 10, 4);
    c.strokeStyle = '#27ae60';
    c.setLineDash([3, 3]);
    c.beginPath();
    c.arc(cx, cy, tolR, 0, Math.PI * 2);
    c.stroke();
    c.setLineDash([]);

    // Bubble dot
    var maxAngle = 10;
    var bx = (roll / maxAngle) * r;
    var by = -(pitch / maxAngle) * r;
    var dist = Math.sqrt(bx * bx + by * by);
    if (dist > r - 8) {
        var s = (r - 8) / dist;
        bx *= s; by *= s;
    }

    var mag = Math.sqrt(pitch * pitch + roll * roll);
    var color = isLevel ? '#2ecc71' : (mag < 2 ? '#f39c12' : '#e74c3c');

    c.fillStyle = color;
    c.beginPath();
    c.arc(cx + bx, cy + by, 8, 0, Math.PI * 2);
    c.fill();
    c.strokeStyle = '#fff';
    c.lineWidth = 1;
    c.stroke();

    // Center dot
    c.fillStyle = '#888';
    c.beginPath();
    c.arc(cx, cy, 2, 0, Math.PI * 2);
    c.fill();
}

// Send commands
function send(obj) {
    if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify(obj));
    }
}

function mv(motor, steps) {
    send({cmd: 'motor', id: motor, steps: steps});
}

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
    send({
        cmd: 'tolerance',
        deg: parseFloat(document.getElementById('tol').value)
    });
}

// Start connection
connect();
