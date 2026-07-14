const boardEl = document.getElementById('board');
const evalBarEl = document.getElementById('eval-bar');
const evalScoreEl = document.getElementById('eval-score');
const statDepthEl = document.getElementById('stat-depth');
const statNodesEl = document.getElementById('stat-nodes');
const statBestMoveEl = document.getElementById('stat-bestmove');
const consoleOutputEl = document.getElementById('console-output');

let board = null;
let game = new Chess();
const wsProtocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
let ws = new WebSocket(`${wsProtocol}//${window.location.host}/ws`);
let selectedSquare = null;

// WebSocket Connection
ws.onopen = () => {
    logToConsole("Connected to BlunderBot server.");
};

ws.onmessage = (event) => {
    const msg = event.data;
    parseEngineOutput(msg);
};

ws.onclose = () => {
    logToConsole("Disconnected from server. Is it running?");
};

// Chessboard.js configuration
function removeHighlights() {
    $('#board .square-55d63').removeClass('legal-move-indicator legal-capture-indicator square-selected');
}

function showLegalMoves(sourceSq) {
    var moves = game.moves({
        square: sourceSq,
        verbose: true
    });
    for (var i = 0; i < moves.length; i++) {
        var toSq = moves[i].to;
        if (moves[i].captured) {
            $('#board .square-' + toSq).addClass('legal-capture-indicator');
        } else {
            $('#board .square-' + toSq).addClass('legal-move-indicator');
        }
    }
}

function onDragStart (source, piece, position, orientation) {
    // do not pick up pieces if the game is over
    if (game.game_over()) return false;
    
    // only pick up pieces for White
    if (piece.search(/^b/) !== -1) return false;

    // Change selection and show legal moves
    if (selectedSquare !== source) {
        removeHighlights();
        selectedSquare = source;
        $('#board .square-' + source).addClass('square-selected');
        showLegalMoves(source);
    }
}

function onDrop (source, target) {
    // If dropped on the same square (a click), keep the piece selected
    if (source === target) {
        return;
    }

    // Attempting a drag drop move
    removeHighlights();
    selectedSquare = null;

    let move = game.move({
        from: source,
        to: target,
        promotion: 'q'
    });

    if (move === null) return 'snapback';

    updateStatus();
    requestEngineMove();
}

// update the board position after the piece snap
function onSnapEnd () {
    board.position(game.fen());
}

// Click to move logic
$(document).on('click', '#board .square-55d63', function() {
    let sq = $(this).attr('data-square');
    
    if (selectedSquare && selectedSquare !== sq) {
        let piece = game.get(sq);
        
        // Clicking another friendly piece switches selection
        if (piece && piece.color === 'w') {
            removeHighlights();
            selectedSquare = sq;
            $('#board .square-' + sq).addClass('square-selected');
            showLegalMoves(sq);
            return;
        }

        // Try to move
        let move = game.move({
            from: selectedSquare,
            to: sq,
            promotion: 'q'
        });

        removeHighlights();
        selectedSquare = null;

        if (move !== null) {
            board.position(game.fen());
            updateStatus();
            requestEngineMove();
        }
    } else if (!selectedSquare) {
        // If clicking a friendly piece from an unselected state
        let piece = game.get(sq);
        if (piece && piece.color === 'w') {
            selectedSquare = sq;
            $('#board .square-' + sq).addClass('square-selected');
            showLegalMoves(sq);
        }
    }
});

function findKing(color) {
    const boardState = game.board();
    for (let r = 0; r < 8; r++) {
        for (let c = 0; c < 8; c++) {
            if (boardState[r][c] && boardState[r][c].type === 'k' && boardState[r][c].color === color) {
                return 'abcdefgh'[c] + (8 - r);
            }
        }
    }
    return null;
}

function updateStatus () {
    let status = '';
    let moveColor = game.turn() === 'b' ? 'Black' : 'White';
    let turnChar = game.turn();

    // Remove all highlights first
    $('#board .square-55d63').removeClass('square-in-check');

    if (game.in_checkmate()) {
        status = 'Game over, ' + moveColor + ' is in checkmate.';
        let kingSq = findKing(turnChar);
        if (kingSq) $('#board .square-' + kingSq).addClass('square-in-check');
    } else if (game.in_draw()) {
        status = 'Game over, drawn position';
    } else {
        status = moveColor + ' to move';
        if (game.in_check()) {
            status += ', ' + moveColor + ' is in check';
            let kingSq = findKing(turnChar);
            if (kingSq) $('#board .square-' + kingSq).addClass('square-in-check');
        }
    }
    
    // Log the status if it's check/mate
    if (game.in_checkmate() || game.in_check() || game.in_draw()) {
        logToConsole("Status: " + status);
    }
}

const config = {
    draggable: true,
    position: 'start',
    onDragStart: onDragStart,
    onDrop: onDrop,
    onSnapEnd: onSnapEnd,
    pieceTheme: 'https://chessboardjs.com/img/chesspieces/wikipedia/{piece}.png'
};

board = Chessboard('board', config);
updateStatus();

// Engine Communication
function requestEngineMove() {
    // Send position
    ws.send("position fen " + game.fen());
    // Start searching
    ws.send("go depth 6"); // Depth 6 for snappy response
}

function parseEngineOutput(msg) {
    logToConsole(msg);

    // Parse 'info depth 5 score cp 66 nodes 4285 pv d2d4'
    if (msg.startsWith("info ")) {
        const parts = msg.split(" ");
        
        let depth = "-";
        let score = 0;
        let nodes = "-";
        let nps = "-";
        
        for (let i = 0; i < parts.length; i++) {
            if (parts[i] === "depth") depth = parts[i+1];
            if (parts[i] === "nodes") nodes = parts[i+1];
            if (parts[i] === "nps") nps = parts[i+1];
            if (parts[i] === "cp") score = parseInt(parts[i+1]);
            if (parts[i] === "mate") {
                score = parseInt(parts[i+1]) > 0 ? 30000 : -30000;
            }
        }
        
        // Update stats
        statDepthEl.textContent = depth;
        statNodesEl.textContent = nodes;
        const statNpsEl = document.getElementById('stat-nps');
        if (statNpsEl) statNpsEl.textContent = nps;
        
        // Update eval bar
        // Score is relative to side to move.
        // If it's black's turn, we flip it so positive is always white.
        let absoluteScore = score;
        if (game.turn() === 'b') {
            absoluteScore = -absoluteScore;
        }
        
        updateEvalBar(absoluteScore);
    }
    
    // Parse 'bestmove d2d4'
    if (msg.startsWith("bestmove ")) {
        const bestmove = msg.split(" ")[1];
        statBestMoveEl.textContent = bestmove;
        
        if (bestmove && bestmove !== "(none)") {
            const from = bestmove.substring(0, 2);
            const to = bestmove.substring(2, 4);
            const promo = bestmove.length > 4 ? bestmove.charAt(4) : null;
            
            game.move({
                from: from,
                to: to,
                promotion: promo || 'q'
            });
            
            board.position(game.fen());
            updateStatus();
        }
    }
}

function updateEvalBar(cp) {
    // Limit to +/- 1000 for calculation
    const maxCp = 1000;
    let clamped = Math.max(-maxCp, Math.min(maxCp, cp));
    
    // Calculate percentage (0 cp = 50%)
    // 1000cp = +50% = 100% white height
    let percent = 50 + (clamped / 20); 
    
    evalBarEl.style.height = percent + '%';
    
    const displayScore = Math.abs(cp / 100).toFixed(2);
    
    // Format text and adjust text color based on who is winning
    if (cp > 0) {
        evalScoreEl.textContent = '+' + displayScore;
        evalScoreEl.style.color = '#333'; // dark text on white background
        evalScoreEl.style.bottom = '10px';
        evalScoreEl.style.top = 'auto';
    } else {
        evalScoreEl.textContent = '-' + displayScore;
        evalScoreEl.style.color = '#ccc'; // light text on dark background
        evalScoreEl.style.top = '10px';
        evalScoreEl.style.bottom = 'auto';
    }
}

function logToConsole(text) {
    const el = document.createElement('div');
    el.className = 'console-line';
    el.textContent = text;
    consoleOutputEl.appendChild(el);
    consoleOutputEl.scrollTop = consoleOutputEl.scrollHeight;
}

// Buttons
document.getElementById('btn-reset').addEventListener('click', () => {
    game.reset();
    board.start();
    updateStatus();
    updateEvalBar(0);
});

document.getElementById('btn-engine').addEventListener('click', () => {
    requestEngineMove();
});
