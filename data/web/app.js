// ============================================================
// app.js — Smart Escape Box Web App
// WebSocket sync temps réel + Web Audio API
// ============================================================

const ENIGMA_NAMES = [
  '', "L'Académie", "Le Labyrinthe", "Le Spectre Radio",
  "La Sentinelle", "La Libération"
];

const PROFILE_NAMES = ['PUBLIC', 'AVANCÉ', 'EXPERT'];
const PROFILE_COLS  = ['#00ff88', '#ffd700', '#ff3030'];

const GESTURE_ICONS = {
  'APPROCHE LENTE': { icon: '👋', label: 'Approche lente' },
  'MAINTIEN 3s':    { icon: '✋', label: 'Maintien stable' },
  'VAGUE RAPIDE':   { icon: '⚡', label: 'Vague rapide' },
  'RECUL RAPIDE':   { icon: '↩️', label: 'Recul rapide' }
};

// ── État global ───────────────────────────────────────────────
let state = {
  enigma: 0, profile: 1, code: null, solved: [],
  connected: false, seqData: [], seqTimer: null,
  freqTarget: 0, potTarget: 0, sonarTarget: 0,
  mode: 0,          // 0 = Escape Game, 1 = Quiz Mode
  quizData: null,   // question en cours
  quizTimerInterval: null,
  quizAnswered: false
};

// ── Web Audio API ─────────────────────────────────────────────
let audioCtx = null;

function getAudioCtx() {
  if (!audioCtx) {
    audioCtx = new (window.AudioContext || window.webkitAudioContext)();
  }
  return audioCtx;
}

// Joue une fréquence pendant `duration` ms avec un type d'onde
function playTone(freq, duration, type = 'sine', volume = 0.3) {
  try {
    const ctx = getAudioCtx();
    if (ctx.state === 'suspended') ctx.resume();
    const osc  = ctx.createOscillator();
    const gain = ctx.createGain();
    osc.connect(gain);
    gain.connect(ctx.destination);
    osc.type = type;
    osc.frequency.setValueAtTime(freq, ctx.currentTime);
    gain.gain.setValueAtTime(volume, ctx.currentTime);
    gain.gain.exponentialRampToValueAtTime(0.0001, ctx.currentTime + duration / 1000);
    osc.start(ctx.currentTime);
    osc.stop(ctx.currentTime + duration / 1000);
  } catch (e) { /* silencieux */ }
}

// Séquences audio synchronisées avec le firmware
const SOUNDS = {
  // Connexion établie
  connected:    () => { playTone(440,80); setTimeout(() => playTone(660,120), 100); setTimeout(() => playTone(880,200), 230); },
  // Enigme résolue
  enigmaSolved: () => { [523,659,784,1047].forEach((f,i) => setTimeout(() => playTone(f,100,'square',0.2), i*90)); },
  // Quiz : question reçue — jingle espiègle
  quizQuestion: () => {
    const notes = [261,329,392,0,523,493,392];
    notes.forEach((f, i) => {
      setTimeout(() => { if (f > 0) playTone(f, 100, 'sine', 0.25); }, i * 110);
    });
  },
  // Quiz : bonne réponse — fanfare
  quizWin: () => {
    const notes = [[329,80],[392,80],[523,80],[659,150],[0,50],[784,250]];
    let t = 0;
    notes.forEach(([f, d]) => {
      setTimeout(() => { if (f > 0) playTone(f, d, 'square', 0.3); }, t);
      t += d + 10;
    });
  },
  // Quiz : mauvaise réponse — descente grave
  quizFail: () => {
    [494, 440, 196].forEach((f, i) => setTimeout(() => playTone(f, 180 + i*50, 'sawtooth', 0.25), i * 110));
  },
  // Quiz : tick timer
  quizTick: () => { playTone(1000, 40, 'square', 0.15); },
  // Quiz : révélation
  quizReveal: () => {
    [261, 329, 392].forEach((f, i) => setTimeout(() => playTone(f, 80, 'sine', 0.2), i * 80));
  },
  // Victoire — mélodie complète
  victory: () => {
    const melody = [
      [261,150],[329,150],[392,150],[523,300],[0,100],
      [392,120],[523,120],[659,400],[0,150],
      [587,120],[523,120],[494,120],[440,120],[392,300],[0,100],
      [329,120],[392,120],[523,200],[0,100],
      [392,80],[440,80],[494,80],[523,80],[587,80],[659,80],[0,60],
      [523,80],[659,80],[784,600]
    ];
    let t = 0;
    melody.forEach(([f, d]) => {
      setTimeout(() => { if (f > 0) playTone(f, d, 'square', 0.25); }, t);
      t += d + 5;
    });
  },
  // Interlude — fanfare de récompense
  interlude: () => {
    const fanfare = [
      [392,150],[523,150],[523,150],[523,150],[0,60],
      [392,100],[329,100],[392,100],[523,300],[0,80],
      [659,100],[587,100],[523,200],[0,60],[784,400]
    ];
    let t = 0;
    fanfare.forEach(([f, d]) => {
      setTimeout(() => { if (f > 0) playTone(f, d, 'square', 0.3); }, t);
      t += d + 5;
    });
  }
};

// ── WebSocket — reconnect robuste ─────────────────────────────
let ws = null;
let wsRetryDelay = 1000;
const WS_MAX_DELAY = 15000;
const WS_URL = `ws://${location.hostname}/ws`;

function connectWS() {
  ws = new WebSocket(WS_URL);

  ws.onopen = () => {
    state.connected = true;
    wsRetryDelay = 1000;    // Reset backoff
    setConnStatus(true);
    showHUD();
    fetchInitialState();
    SOUNDS.connected();
    startHeartbeat();
  };

  ws.onclose = () => {
    state.connected = false;
    setConnStatus(false);
    showWait();
    stopHeartbeat();
    // Reconnect exponentiel
    setTimeout(connectWS, wsRetryDelay);
    wsRetryDelay = Math.min(wsRetryDelay * 1.5, WS_MAX_DELAY);
  };

  ws.onerror = () => { ws.close(); };

  ws.onmessage = (evt) => {
    try {
      const msg = JSON.parse(evt.data);
      handleMessage(msg);
    } catch (e) { /* ignore */ }
  };
}

// Heartbeat : pong toutes les 10s pour garder la connexion active
let heartbeatTimer = null;
function startHeartbeat() {
  heartbeatTimer = setInterval(() => {
    if (ws && ws.readyState === WebSocket.OPEN) {
      ws.send(JSON.stringify({ type: 'ping' }));
    }
  }, 10000);
}
function stopHeartbeat() {
  if (heartbeatTimer) { clearInterval(heartbeatTimer); heartbeatTimer = null; }
}

async function fetchInitialState() {
  try {
    const r = await fetch('/api/state');
    const d = await r.json();
    applyState(d);
  } catch (e) { /* ignore */ }
}

// ── Gestion des messages ──────────────────────────────────────
function handleMessage(msg) {
  // Dispatch selon le type
  switch (msg.type) {
    case 'state':
    case 'update':
      applyState(msg);
      break;
    case 'quiz_question':
      onQuizQuestion(msg);
      break;
    case 'quiz_result':
      onQuizResult(msg);
      break;
    case 'pong':
      break;  // heartbeat ACK
  }
}

function applyState(data) {
  if (data.profile !== undefined) {
    state.profile = data.profile;
    const badge = document.getElementById('profile-badge');
    badge.textContent = PROFILE_NAMES[state.profile] || '---';
    badge.style.borderColor = PROFILE_COLS[state.profile];
    badge.style.color = PROFILE_COLS[state.profile];
  }

  if (data.mode !== undefined) {
    state.mode = data.mode;
    updateModeDisplay();
  }

  if (data.enigma !== undefined) {
    const prev = state.enigma;
    state.enigma = data.enigma;
    if (data.enigma !== prev) updateEnigmaPanel(data.enigma);
    updateProgress(state.solved, data.enigma);
  }

  if (data.phase === 'start' && data.enigma !== undefined) {
    updateEnigmaPanel(data.enigma);
  }

  if (data.code !== undefined) {
    state.code = data.code;
    updateCodeDisplay(data.code);
  }

  if (data.solved !== undefined) {
    if (!state.solved.includes(data.solved)) state.solved.push(data.solved);
    updateProgress(state.solved, state.enigma);
    onEnigmaSolved(data.solved);
  }

  if (data.event === 'victory') showVictory();

  // Données spécifiques
  if (data.seqGestures) { state.seqData = data.seqGestures; showGestureSeq(); }
  if (data.freqTarget)  { state.freqTarget = data.freqTarget; updateFreq(); }
  if (data.potTarget)   { state.potTarget  = data.potTarget;  updateTargets(); }
  if (data.sonarTarget) { state.sonarTarget= data.sonarTarget;updateTargets(); }
}

// ── Quiz — réception d'une question ──────────────────────────
function onQuizQuestion(msg) {
  state.quizData    = msg;
  state.quizAnswered = false;

  // Afficher le panel quiz avec transition
  SOUNDS.interlude();
  setTimeout(() => {
    SOUNDS.quizQuestion();
    showQuizPanel(msg);
  }, 1800);
}

function showQuizPanel(msg) {
  const content = document.getElementById('content-area');
  content.style.opacity = '0';
  setTimeout(() => {
    showPanel('panel-quiz');
    renderQuizQuestion(msg);
    content.style.transition = 'opacity 0.5s ease';
    content.style.opacity    = '1';
  }, 300);
  document.getElementById('enigma-label').textContent = '⚡ ÉPREUVE INTELLECTUELLE';
  document.getElementById('enigma-name').textContent  = msg.domain || 'Défi';
}

function renderQuizQuestion(msg) {
  // Domaine
  document.getElementById('quiz-domain-badge').textContent = `[ ${msg.domain} ]`;

  // Question
  document.getElementById('quiz-question').textContent = msg.question;

  // Cacher résultat précédent
  document.getElementById('quiz-result').classList.add('hidden');
  document.getElementById('quiz-explanation').classList.add('hidden');

  // Boutons de réponse
  const container = document.getElementById('quiz-choices');
  container.innerHTML = '';
  const choices = msg.choices || [];
  choices.forEach((choice, idx) => {
    const btn = document.createElement('button');
    btn.className = 'quiz-btn';
    btn.id = `quiz-choice-${idx}`;
    btn.setAttribute('data-idx', idx);
    // Lettre + texte
    const letter = String.fromCharCode(65 + idx);  // A, B, C...
    btn.innerHTML = `<span class="quiz-btn-letter">${letter}</span><span class="quiz-btn-text">${choice}</span>`;
    btn.addEventListener('click', () => submitQuizAnswer(idx));
    // Animation d'entrée décalée
    btn.style.animationDelay = `${idx * 80}ms`;
    container.appendChild(btn);
  });

  // Timer
  const totalMs = (msg.difficulty === 1) ? 30000 : (msg.difficulty === 2) ? 40000 : 50000;
  startQuizTimer(totalMs);
}

// ── Soumission de réponse depuis le téléphone ─────────────────
function submitQuizAnswer(idx) {
  if (state.quizAnswered) return;
  // Envoi WebSocket
  if (ws && ws.readyState === WebSocket.OPEN) {
    ws.send(JSON.stringify({ type: 'quiz_answer', choice: idx }));
  }
  // Marquer visuellement en attente
  document.querySelectorAll('.quiz-btn').forEach(b => b.disabled = true);
  document.getElementById(`quiz-choice-${idx}`).classList.add('quiz-btn--selected');
}

// ── Résultat quiz reçu du firmware ───────────────────────────
function onQuizResult(msg) {
  state.quizAnswered = true;
  stopQuizTimer();

  const correct    = msg.correct;
  const correctIdx = msg.correctIdx;

  // Colorer les boutons
  document.querySelectorAll('.quiz-btn').forEach((btn, i) => {
    btn.disabled = true;
    if (i === correctIdx) {
      btn.classList.add('quiz-btn--correct');
    } else if (btn.classList.contains('quiz-btn--selected') && !correct) {
      btn.classList.add('quiz-btn--wrong');
    }
  });

  // Afficher résultat
  const resultEl = document.getElementById('quiz-result');
  resultEl.classList.remove('hidden', 'quiz-result--win', 'quiz-result--fail');
  if (correct) {
    resultEl.textContent = '✅ EXACT !';
    resultEl.classList.add('quiz-result--win');
    SOUNDS.quizWin();
    document.body.classList.add('flash-win');
    setTimeout(() => document.body.classList.remove('flash-win'), 600);
  } else {
    resultEl.textContent = '❌ INCORRECT';
    resultEl.classList.add('quiz-result--fail');
    SOUNDS.quizFail();
    document.body.classList.add('flash-fail');
    setTimeout(() => document.body.classList.remove('flash-fail'), 600);
  }

  // Explication
  if (state.quizData && state.quizData.explanation) {
    setTimeout(() => {
      SOUNDS.quizReveal();
      const expEl = document.getElementById('quiz-explanation');
      expEl.textContent = '💡 ' + state.quizData.explanation;
      expEl.classList.remove('hidden');
    }, correct ? 500 : 1200);
  }
}

// ── Timer quiz ────────────────────────────────────────────────
function startQuizTimer(totalMs) {
  const fill = document.getElementById('quiz-timer-fill');
  const start = Date.now();
  stopQuizTimer();
  let lastTickSec = -1;

  state.quizTimerInterval = setInterval(() => {
    const elapsed = Date.now() - start;
    const pct = Math.max(0, 1 - elapsed / totalMs);
    fill.style.width = `${pct * 100}%`;

    // Couleur du timer
    if (pct > 0.5)      fill.style.background = 'var(--col-terminal)';
    else if (pct > 0.25) fill.style.background = 'var(--col-gold)';
    else                 fill.style.background = 'var(--col-danger)';

    // Tick sonore dans les 5 dernières secondes
    const secondsLeft = Math.ceil((totalMs - elapsed) / 1000);
    if (secondsLeft <= 5 && secondsLeft !== lastTickSec && secondsLeft > 0) {
      SOUNDS.quizTick();
      lastTickSec = secondsLeft;
    }

    if (elapsed >= totalMs) stopQuizTimer();
  }, 100);
}

function stopQuizTimer() {
  if (state.quizTimerInterval) {
    clearInterval(state.quizTimerInterval);
    state.quizTimerInterval = null;
  }
}

// ── Panneaux principaux ───────────────────────────────────────
function hideAllPanels() {
  document.querySelectorAll('.panel').forEach(p => p.classList.add('hidden'));
  stopQuizTimer();
}

function showPanel(id) {
  hideAllPanels();
  const el = document.getElementById(id);
  if (el) el.classList.remove('hidden');
}

function updateModeDisplay() {
  // Mode quiz autonome → afficher le panel quiz directement
  if (state.mode === 1) {
    document.getElementById('enigma-label').textContent = 'MODE DÉFI';
    document.getElementById('enigma-name').textContent  = 'Quiz en cours...';
  }
}

function updateEnigmaPanel(n) {
  const label = document.getElementById('enigma-label');
  const name  = document.getElementById('enigma-name');
  label.textContent = (n > 0 && n < 6) ? `PROTOCOLE 0${n}` : 'PROTOCOLE --';
  name.textContent  = ENIGMA_NAMES[n] || '---';

  const content = document.getElementById('content-area');
  content.style.opacity = '0';
  setTimeout(() => {
    switch (n) {
      case 2: showPanel('panel-maze');       break;
      case 3: showPanel('panel-radio');      break;
      case 4: showPanel('panel-sentinel');   break;
      case 5: showPanel('panel-liberation'); break;
      default: showPanel('panel-idle');      break;
    }
    content.style.transition = 'opacity 0.5s ease';
    content.style.opacity    = '1';
  }, 300);
}

function onEnigmaSolved(n) {
  SOUNDS.enigmaSolved();
  document.body.style.background = '#001a0a';
  setTimeout(() => document.body.style.background = '', 500);
}

function showVictory() {
  hideAllPanels();
  document.getElementById('panel-victory').classList.remove('hidden');
  document.getElementById('enigma-label').textContent = '🔓 DÉVERROUILLÉ';
  document.getElementById('enigma-name').textContent  = 'La Boîte est Ouverte';
  SOUNDS.victory();
  launchConfetti();
}

// ── Code display ──────────────────────────────────────────────
function updateCodeDisplay(code) {
  const enc = document.getElementById('encoded-msg');
  if (enc) enc.textContent = String(code).padStart(4, '0');
  const fin = document.getElementById('final-code');
  if (fin) fin.textContent = String(code).padStart(4, '0');
}

function updateFreq() {
  const el = document.getElementById('freq-target');
  if (el) el.textContent = `SYNTONISE SUR : ${state.freqTarget} Hz`;
  buildDecodeTable();
}

function buildDecodeTable() {
  const grid = document.getElementById('decode-table');
  if (!grid) return;
  grid.innerHTML = '';
  const shift = [0, 3, 5][state.profile] || 3;
  for (let i = 0; i <= 9; i++) {
    const encoded = (i + shift) % 10;
    const cell = document.createElement('div');
    cell.className = 'decode-cell';
    cell.innerHTML = `${encoded}<span>${i}</span>`;
    grid.appendChild(cell);
  }
}

function updateTargets() {
  const pot   = document.getElementById('pot-target');
  const sonar = document.getElementById('sonar-target');
  if (pot)   pot.textContent   = `Zone ${Math.round(state.potTarget / 40.95)}%`;
  if (sonar) sonar.textContent = `${state.sonarTarget.toFixed(1)} cm`;
}

// ── Séquence de gestes ────────────────────────────────────────
function showGestureSeq() {
  const list = document.getElementById('gesture-seq');
  if (!list) return;
  list.innerHTML = '';
  state.seqData.forEach((g, i) => {
    const info = GESTURE_ICONS[g] || { icon: '❓', label: g };
    const item = document.createElement('div');
    item.className = 'gest-item';
    item.innerHTML = `<span class="gest-num">${i+1}</span>
                      <span class="gest-icon">${info.icon}</span>
                      <span>${info.label}</span>`;
    list.appendChild(item);
  });
  const fill = document.getElementById('seq-timer-fill');
  const hint = document.getElementById('seq-hint');
  let start = Date.now();
  const dur = 8000;
  clearInterval(state.seqTimer);
  state.seqTimer = setInterval(() => {
    const elapsed = Date.now() - start;
    const pct = Math.max(0, 1 - elapsed / dur);
    if (fill) fill.style.width = `${pct * 100}%`;
    if (elapsed >= dur) {
      clearInterval(state.seqTimer);
      if (list) list.innerHTML = '<p style="text-align:center;color:#4a5568">Séquence masquée</p>';
      if (hint) hint.textContent = 'Reproduisez les gestes !';
    }
  }, 100);
}

// ── Confettis victoire ────────────────────────────────────────
function launchConfetti() {
  const canvas = document.createElement('canvas');
  canvas.style.cssText = 'position:fixed;top:0;left:0;width:100%;height:100%;pointer-events:none;z-index:999';
  document.body.appendChild(canvas);
  const ctx = canvas.getContext('2d');
  canvas.width = window.innerWidth;
  canvas.height = window.innerHeight;
  const particles = Array.from({length: 80}, () => ({
    x: Math.random() * canvas.width, y: -10,
    vx: (Math.random() - 0.5) * 4,
    vy: 2 + Math.random() * 4,
    color: ['#ffd700','#00ff88','#ff6b6b','#4ecdc4'][Math.floor(Math.random()*4)],
    size: 4 + Math.random() * 6
  }));
  let frame = 0;
  function anim() {
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    particles.forEach(p => {
      p.x += p.vx; p.y += p.vy; p.vy += 0.1;
      ctx.fillStyle = p.color;
      ctx.beginPath();
      ctx.arc(p.x, p.y, p.size, 0, Math.PI * 2);
      ctx.fill();
    });
    frame++;
    if (frame < 200) requestAnimationFrame(anim);
    else canvas.remove();
  }
  anim();
}

// ── UI helpers ────────────────────────────────────────────────
function setConnStatus(online) {
  const dot   = document.getElementById('conn-dot');
  const label = document.getElementById('conn-label');
  dot.className     = `dot ${online ? 'online' : 'offline'}`;
  label.textContent = online ? 'CONNEXION ÉTABLIE' : 'Reconnexion...';
}

function showWait() {
  document.getElementById('screen-wait').classList.add('active');
  document.getElementById('screen-hud').classList.remove('active');
}

function showHUD() {
  document.getElementById('screen-wait').classList.remove('active');
  document.getElementById('screen-hud').classList.add('active');
  showPanel('panel-idle');
}

function updateProgress(solved, current) {
  for (let i = 1; i <= 5; i++) {
    const el = document.getElementById(`step-${i}`);
    if (!el) continue;
    el.className = 'prog-step';
    if (solved.includes(i))  el.classList.add('done');
    else if (i === current)  el.classList.add('active');
  }
}

// ── Débloquer l'AudioContext au premier touch ─────────────────
document.addEventListener('touchstart', () => {
  if (!audioCtx) getAudioCtx();
  else if (audioCtx.state === 'suspended') audioCtx.resume();
}, { once: false, passive: true });

document.addEventListener('click', () => {
  if (audioCtx && audioCtx.state === 'suspended') audioCtx.resume();
}, { passive: true });

// ── Boot ──────────────────────────────────────────────────────
window.addEventListener('load', () => {
  connectWS();
});
