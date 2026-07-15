import subprocess
import sys
import os

# Known perft results: (FEN, Depth, Expected Nodes)
TEST_POSITIONS = [
    ("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 4, 197281), # Start pos
    ("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 3, 97862), # Kiwipete
    ("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 4, 43238), # Position 3
    ("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 4, 422333), # Position 4
    ("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 3, 62379) # Position 5
]

def main():
    engine_path = ""
    if os.path.exists(os.path.join("build", "Release", "BlunderBot.exe")):
        engine_path = os.path.join("build", "Release", "BlunderBot.exe")
    elif os.path.exists(os.path.join("build", "BlunderBot")):
        engine_path = os.path.join("build", "BlunderBot")
    elif os.path.exists(os.path.join("build", "BlunderBot.exe")):
        engine_path = os.path.join("build", "BlunderBot.exe")
        
    if not engine_path:
        print(f"Error: Engine not found at {engine_path}")
        sys.exit(1)

    all_passed = True

    for fen, depth, expected in TEST_POSITIONS:
        print(f"Testing FEN: {fen} at depth {depth}")
        
        process = subprocess.Popen(
            [engine_path],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        
        # Send position and perft command
        cmds = f"uci\nposition fen {fen}\nperft {depth}\nquit\n"
        output, _ = process.communicate(input=cmds)
        
        # Parse output
        actual = -1
        for line in output.split('\n'):
            if "Nodes searched:" in line:
                try:
                    actual = int(line.split(":")[-1].strip())
                except:
                    pass
        
        if actual == expected:
            print(f"  [PASS] Expected {expected}, got {actual}")
        else:
            print(f"  [FAIL] Expected {expected}, got {actual}")
            print(f"  [DEBUG] Raw output:\n{output}")
            all_passed = False

    if all_passed:
        print("\nAll Perft tests passed successfully!")
        sys.exit(0)
    else:
        print("\nSome Perft tests failed!")
        sys.exit(1)

if __name__ == "__main__":
    main()
