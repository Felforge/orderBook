import random

# Set seed for reproducibility
random.seed(42)

# Generate 100,000 decisions
# 1 = submit (60%), 0 = cancel (40%)
N = 100000
decisions = [1 if random.random() < 0.6 else 0 for _ in range(N)]

# Verify distribution
num_ones = sum(decisions)
num_zeros = N - num_ones
print(f"Generated {N} decisions:")
print(f"  1s (submits): {num_ones} ({100*num_ones/N:.2f}%)")
print(f"  0s (cancels): {num_zeros} ({100*num_zeros/N:.2f}%)")

# Save to file - one number per line for easy C++ reading
with open('decisions.txt', 'w') as f:
    for decision in decisions:
        f.write(f"{decision}\n")

print(f"\nSaved to decisions.txt")
