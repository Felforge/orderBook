import numpy as np
import matplotlib.pyplot as plt
import random

# Set seed for reproducibility
np.random.seed(42)
random.seed(42)

# Parameters for stock price simulation (HFT scenario)
N = 100000  # Number of time steps
INITIAL_PRICE = 100.0  # Starting stock price
MU = 0.0  # Drift (no trend in short HFT timeframe)
SIGMA = 0.00005  # Volatility (very small for tight HFT price action)
DT = 1.0  # Time step (in arbitrary units)

# Generate stock price using Geometric Brownian Motion
def generate_stock_price(n, initial_price, mu, sigma, dt):
    """
    Geometric Brownian Motion: dS = μS dt + σS dW
    where W is a Wiener process (Brownian motion)
    """
    local_prices = np.zeros(n)
    local_prices[0] = initial_price
    for idx in range(1, n):
        shock = np.random.normal(0, 1) * np.sqrt(dt)
        drift = mu * local_prices[idx-1] * dt
        diffusion = sigma * local_prices[idx-1] * shock
        local_prices[idx] = local_prices[idx-1] + drift + diffusion
        local_prices[idx] = max(local_prices[idx], 0.01)
    return local_prices

# Generate realistic bid-ask spread
def generate_spread(spread_price, base_spread=0.01):
    """Generate bid-ask spread that widens with volatility"""
    spread = base_spread * spread_price * (1 + 0.5 * np.random.random())
    return spread

# Generate order flow based on price movements
def generate_orders(orders_prices):
    """
    Generate buy/sell orders based on price momentum and mean reversion
    1 = Buy order, 0 = Sell order
    """
    local_orders = []
    for idx in range(len(orders_prices)):
        if idx == 0:
            local_orders.append(random.choice([0, 1]))
            continue
        price_change = orders_prices[idx] - orders_prices[idx-1]
        if price_change < -0.05:
            prob_buy = 0.7
        elif price_change > 0.05:
            prob_buy = 0.3
        else:
            prob_buy = 0.5
        if idx % 1000 < 500:
            if idx > 100:
                recent_trend = orders_prices[idx] - orders_prices[idx-100]
                if recent_trend > 0:
                    prob_buy += 0.1
                else:
                    prob_buy -= 0.1
        prob_buy = max(0.1, min(0.9, prob_buy))
        local_orders.append(1 if random.random() < prob_buy else 0)
    return local_orders

# Generate trade sizes
def generate_sizes(n):
    """Generate realistic order sizes using log-normal distribution"""
    # Log-normal distribution gives realistic heavy-tailed size distribution
    raw_sizes = np.random.lognormal(mean=4, sigma=1, size=n)
    final_sizes = np.maximum(np.round(raw_sizes), 1).astype(int)
    return final_sizes

print("Generating realistic stock price movements...")
prices = generate_stock_price(N, INITIAL_PRICE, MU, SIGMA, DT)

print("Generating order flow...")
orders = generate_orders(prices)

print("Generating order sizes...")
sizes = generate_sizes(N)

# Calculate statistics
num_buys = sum(orders)
num_sells = N - num_buys
avg_price = np.mean(prices)
price_volatility = np.std(prices)
price_min = np.min(prices)
price_max = np.max(prices)

print("\n" + "="*60)
print("Market Data Statistics:")
print("="*60)
print(f"Total orders: {N:,}")
print(f"  Buy orders:  {num_buys:,} ({100*num_buys/N:.2f}%)")
print(f"  Sell orders: {num_sells:,} ({100*num_sells/N:.2f}%)")
print("\nPrice Statistics:")
print(f"  Initial price:  ${INITIAL_PRICE:.2f}")
print(f"  Final price:    ${prices[-1]:.2f}")
print(f"  Average price:  ${avg_price:.2f}")
print(f"  Min price:      ${price_min:.2f}")
print(f"  Max price:      ${price_max:.2f}")
print(f"  Volatility:     ${price_volatility:.2f}")
print(f"  Total return:   {100*(prices[-1]-INITIAL_PRICE)/INITIAL_PRICE:.2f}%")
print("\nOrder Size Statistics:")
print(f"  Average size:   {np.mean(sizes):.0f} shares")
print(f"  Median size:    {np.median(sizes):.0f} shares")
print(f"  Max size:       {np.max(sizes):.0f} shares")
print("="*60 + "\n")

# Save data file
print("Saving market data...")

# Save combined data
with open('market_data.txt', 'w', encoding='utf-8') as f:
    f.write("# Order_Type Price Size\n")
    f.write("# Order_Type: 1=Buy, 0=Sell\n")
    for i in range(N):
        f.write(f"{orders[i]} {prices[i]:.2f} {sizes[i]}\n")

print("Creating visualizations...")

# Create comprehensive visualization
fig = plt.figure(figsize=(16, 12))

# Plot 1: Stock Price Over Time
ax1 = plt.subplot(3, 2, 1)
time_steps = np.arange(N)
ax1.plot(time_steps, prices, linewidth=0.5, color='blue', alpha=0.7)
ax1.set_title('Stock Price Movement (Geometric Brownian Motion)', fontsize=12, fontweight='bold')
ax1.set_xlabel('Time Step')
ax1.set_ylabel('Price ($)')
ax1.grid(True, alpha=0.3)
ax1.axhline(y=INITIAL_PRICE, color='r', linestyle='--', linewidth=1, alpha=0.5, label=f'Initial: ${INITIAL_PRICE}')
ax1.legend()

# Plot 2: Price with Buy/Sell Signals (sample for visibility)
ax2 = plt.subplot(3, 2, 2)
# Show only first 5000 points for clarity
sample_n = min(5000, N)
ax2.plot(time_steps[:sample_n], prices[:sample_n], linewidth=1, color='black', alpha=0.5, label='Price')
buy_indices = [i for i in range(sample_n) if orders[i] == 1]
sell_indices = [i for i in range(sample_n) if orders[i] == 0]
ax2.scatter(buy_indices, prices[buy_indices], color='green', s=10, alpha=0.6, label='Buy', marker='^')
ax2.scatter(sell_indices, prices[sell_indices], color='red', s=10, alpha=0.6, label='Sell', marker='v')
ax2.set_title(f'Price with Buy/Sell Signals (First {sample_n} orders)', fontsize=12, fontweight='bold')
ax2.set_xlabel('Time Step')
ax2.set_ylabel('Price ($)')
ax2.legend()
ax2.grid(True, alpha=0.3)

# Plot 3: Price Distribution (Histogram)
ax3 = plt.subplot(3, 2, 3)
ax3.hist(prices, bins=100, color='skyblue', edgecolor='black', alpha=0.7)
ax3.axvline(x=avg_price, color='r', linestyle='--', linewidth=2, label=f'Mean: ${avg_price:.2f}')
ax3.set_title('Price Distribution', fontsize=12, fontweight='bold')
ax3.set_xlabel('Price ($)')
ax3.set_ylabel('Frequency')
ax3.legend()
ax3.grid(True, alpha=0.3, axis='y')

# Plot 4: Rolling Statistics
ax4 = plt.subplot(3, 2, 4)
window = 1000
rolling_mean = np.convolve(prices, np.ones(window)/window, mode='valid')
rolling_std = np.array([np.std(prices[max(0,i-window):i+1]) for i in range(window-1, N)])
# Ensure arrays have same length
min_len = min(len(time_steps[window-1:]), len(rolling_mean), len(rolling_std))
ax4.plot(time_steps[window-1:window-1+min_len], rolling_mean[:min_len], label=f'{window}-step Moving Average', linewidth=1.5, color='orange')
ax4.fill_between(time_steps[window-1:window-1+min_len],
                  rolling_mean[:min_len] - rolling_std[:min_len],
                  rolling_mean[:min_len] + rolling_std[:min_len],
                  alpha=0.3, color='orange', label='±1 Std Dev')
ax4.set_title(f'Rolling Mean and Volatility (Window={window})', fontsize=12, fontweight='bold')
ax4.set_xlabel('Time Step')
ax4.set_ylabel('Price ($)')
ax4.legend()
ax4.grid(True, alpha=0.3)

# Plot 5: Order Type Distribution Over Time
ax5 = plt.subplot(3, 2, 5)
window = 1000
buy_ratio = np.convolve(orders, np.ones(window)/window, mode='valid')
min_len_buy = min(len(time_steps[window-1:]), len(buy_ratio))
ax5.plot(time_steps[window-1:window-1+min_len_buy], buy_ratio[:min_len_buy] * 100, linewidth=1, color='purple')
ax5.axhline(y=50, color='gray', linestyle='--', linewidth=1, alpha=0.5)
ax5.set_title(f'Buy Order Percentage (Rolling {window}-step window)', fontsize=12, fontweight='bold')
ax5.set_xlabel('Time Step')
ax5.set_ylabel('Buy Orders (%)')
ax5.set_ylim([0, 100])
ax5.grid(True, alpha=0.3)

# Plot 6: Order Size Distribution
ax6 = plt.subplot(3, 2, 6)
ax6.hist(sizes, bins=100, color='lightcoral', edgecolor='black', alpha=0.7)
ax6.axvline(x=np.mean(sizes), color='r', linestyle='--', linewidth=2, label=f'Mean: {np.mean(sizes):.0f}')
ax6.axvline(x=np.median(sizes), color='b', linestyle='--', linewidth=2, label=f'Median: {np.median(sizes):.0f}')
ax6.set_title('Order Size Distribution (Log-Normal)', fontsize=12, fontweight='bold')
ax6.set_xlabel('Shares')
ax6.set_ylabel('Frequency')
ax6.legend()
ax6.grid(True, alpha=0.3, axis='y')
ax6.set_xlim([0, np.percentile(sizes, 99)])  # Zoom to 99th percentile for better visibility

plt.tight_layout()
plt.savefig('market_visualization.png', dpi=300, bbox_inches='tight')
print("Saved visualization to market_visualization.png")

# Create a second figure focusing on price action
fig2 = plt.figure(figsize=(16, 10))

# Detailed price chart with different views
ax1 = plt.subplot(2, 2, 1)
ax1.plot(time_steps[:10000], prices[:10000], linewidth=0.8, color='blue')
ax1.set_title('Price Movement (First 10,000 steps)', fontsize=12, fontweight='bold')
ax1.set_xlabel('Time Step')
ax1.set_ylabel('Price ($)')
ax1.grid(True, alpha=0.3)

ax2 = plt.subplot(2, 2, 2)
ax2.plot(time_steps[-10000:], prices[-10000:], linewidth=0.8, color='red')
ax2.set_title('Price Movement (Last 10,000 steps)', fontsize=12, fontweight='bold')
ax2.set_xlabel('Time Step')
ax2.set_ylabel('Price ($)')
ax2.grid(True, alpha=0.3)

# Returns distribution
ax3 = plt.subplot(2, 2, 3)
returns = np.diff(prices) / prices[:-1] * 100  # Percentage returns
ax3.hist(returns, bins=100, color='green', edgecolor='black', alpha=0.7)
ax3.set_title('Returns Distribution', fontsize=12, fontweight='bold')
ax3.set_xlabel('Return (%)')
ax3.set_ylabel('Frequency')
ax3.grid(True, alpha=0.3, axis='y')
ax3.axvline(x=0, color='r', linestyle='--', linewidth=2)

# Cumulative returns
ax4 = plt.subplot(2, 2, 4)
cumulative_returns = (prices / INITIAL_PRICE - 1) * 100
ax4.plot(time_steps, cumulative_returns, linewidth=1, color='darkgreen')
ax4.set_title('Cumulative Returns', fontsize=12, fontweight='bold')
ax4.set_xlabel('Time Step')
ax4.set_ylabel('Cumulative Return (%)')
ax4.grid(True, alpha=0.3)
ax4.axhline(y=0, color='r', linestyle='--', linewidth=1, alpha=0.5)

plt.tight_layout()
plt.savefig('price_analysis.png', dpi=300, bbox_inches='tight')
print("Saved price analysis to price_analysis.png")

print("\n✓ All files generated successfully!")
print("\nGenerated files:")
print("  - market_data.txt (order data for C++ tests)")
print("  - market_visualization.png (comprehensive charts)")
print("  - price_analysis.png (detailed price charts)")
