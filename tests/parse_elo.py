import sys
import os
import re

def main():
    if len(sys.argv) < 2:
        print("Usage: python parse_elo.py <cutechess.out>")
        sys.exit(1)
        
    filename = sys.argv[1]
    if not os.path.exists(filename):
        print(f"File not found: {filename}")
        sys.exit(1)
        
    with open(filename, 'r') as f:
        content = f.read()
        
    # Check if H0 was accepted (PR is worse)
    if "H0 accepted" in content or "H0 was accepted" in content:
        print("SPRT Failed: H0 was accepted. The new code is statistically proven to be worse.")
        sys.exit(1)
        
    # Check if H1 was accepted (PR is better)
    if "H1 accepted" in content or "H1 was accepted" in content:
        print("SPRT Passed: H1 was accepted. The new code is statistically proven to be equal or better.")
        sys.exit(0)
        
    # If neither, the test hit the max game limit. We must parse the LOS (Likelihood of Superiority).
    match = re.search(r'LOS:\s*([\d\.]+)%', content)
    if match:
        los = float(match.group(1))
        print(f"SPRT Inconclusive. Likelihood of Superiority: {los}%")
        if los < 50.0:
            print("LOS is less than 50%. Assuming the new code is worse.")
            sys.exit(1)
        else:
            print("LOS is >= 50%. Assuming the new code is acceptable.")
            sys.exit(0)
    # Try parsing LLR from c-chess-cli
    llrs = re.findall(r'SPRT:\s*LLR\s*=\s*([-.\d]+)', content)
    if llrs:
        last_llr = float(llrs[-1])
        print(f"SPRT Inconclusive. Last LLR: {last_llr}")
        if last_llr < 0.0:
            print("LLR is negative. Assuming the new code is worse.")
            sys.exit(1)
        else:
            print("LLR is >= 0. Assuming the new code is acceptable.")
            sys.exit(0)
            
    print("Could not parse SPRT or LOS from output. The test may have crashed.")
    sys.exit(1)

if __name__ == "__main__":
    main()
