import http.server
import socketserver
import os
import webbrowser
import threading

PORT = 8080
DIRECTORY = os.path.dirname(os.path.abspath(__file__))

HTML_CONTENT = """
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Tetris AI - Training Dashboard</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
        @import url('https://fonts.googleapis.com/css2?family=Inter:wght@400;600;800&display=swap');
        
        body {
            background-color: #0f172a;
            color: #f8fafc;
            font-family: 'Inter', sans-serif;
            margin: 0;
            padding: 2rem;
            display: flex;
            flex-direction: column;
            align-items: center;
        }
        
        .header {
            text-align: center;
            margin-bottom: 2rem;
        }
        
        h1 {
            font-size: 2.5rem;
            font-weight: 800;
            background: -webkit-linear-gradient(45deg, #3b82f6, #8b5cf6);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            margin: 0 0 0.5rem 0;
        }
        
        p.subtitle {
            color: #94a3b8;
            font-size: 1.1rem;
            margin: 0;
        }
        
        .stats-container {
            display: flex;
            gap: 1.5rem;
            margin-bottom: 2rem;
            width: 100%;
            max-width: 1200px;
        }
        
        .stat-card {
            background: #1e293b;
            border-radius: 1rem;
            padding: 1.5rem;
            flex: 1;
            box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.1), 0 2px 4px -1px rgba(0, 0, 0, 0.06);
            border: 1px solid #334155;
            transition: transform 0.2s;
        }
        
        .stat-card:hover {
            transform: translateY(-2px);
        }
        
        .stat-title {
            color: #94a3b8;
            font-size: 0.875rem;
            text-transform: uppercase;
            letter-spacing: 0.05em;
            margin-bottom: 0.5rem;
            font-weight: 600;
        }
        
        .stat-value {
            font-size: 2rem;
            font-weight: 800;
            color: #f8fafc;
        }
        
        .chart-container {
            background: #1e293b;
            border-radius: 1rem;
            padding: 1.5rem;
            width: 100%;
            max-width: 1200px;
            box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.1);
            border: 1px solid #334155;
            height: 60vh;
        }
        
        .status {
            margin-top: 1rem;
            font-size: 0.875rem;
            color: #10b981;
            display: flex;
            align-items: center;
            gap: 0.5rem;
        }
        
        .pulse {
            width: 8px;
            height: 8px;
            background-color: #10b981;
            border-radius: 50%;
            animation: pulse-animation 2s infinite;
        }
        
        @keyframes pulse-animation {
            0% { box-shadow: 0 0 0 0 rgba(16, 185, 129, 0.7); }
            70% { box-shadow: 0 0 0 10px rgba(16, 185, 129, 0); }
            100% { box-shadow: 0 0 0 0 rgba(16, 185, 129, 0); }
        }
    </style>
</head>
<body>
    <div class="header">
        <h1>Tetris AI Training Dashboard</h1>
        <p class="subtitle">Live CMA-ES Optimization Tracking</p>
    </div>
    
    <div class="stats-container">
        <div class="stat-card">
            <div class="stat-title">Current Generation</div>
            <div class="stat-value" id="stat-gen">--</div>
        </div>
        <div class="stat-card">
            <div class="stat-title">Best Lines Cleared</div>
            <div class="stat-value" id="stat-best" style="color: #10b981;">--</div>
        </div>
        <div class="stat-card">
            <div class="stat-title">Median Lines Cleared</div>
            <div class="stat-value" id="stat-median" style="color: #3b82f6;">--</div>
        </div>
        <div class="stat-card">
            <div class="stat-title">Time per Gen (Avg)</div>
            <div class="stat-value" id="stat-time">-- s</div>
        </div>
    </div>

    <div class="chart-container">
        <canvas id="trainingChart"></canvas>
    </div>
    
    <div class="status">
        <div class="pulse"></div>
        Auto-updating every 5 seconds
    </div>

    <script>
        let chart = null;
        
        async function fetchLog() {
            try {
                // Add timestamp to prevent caching
                const response = await fetch('/training_log.csv?t=' + new Date().getTime());
                if (!response.ok) return;
                
                const csvText = await response.text();
                const lines = csvText.trim().split('\\n');
                if (lines.length <= 1) return;
                
                const labels = [];
                const bestData = [];
                const medianData = [];
                const worstData = [];
                let totalTime = 0;
                
                for (let i = 1; i < lines.length; i++) {
                    const parts = lines[i].split(',');
                    if (parts.length >= 5) {
                        labels.push('Gen ' + parts[0]);
                        bestData.push(parseFloat(parts[1]));
                        medianData.push(parseFloat(parts[2]));
                        worstData.push(parseFloat(parts[3]));
                        totalTime += parseFloat(parts[4]);
                    }
                }
                
                const currentGen = labels.length;
                const latestBest = bestData[bestData.length - 1];
                const latestMedian = medianData[medianData.length - 1];
                const avgTime = (totalTime / currentGen).toFixed(1);
                
                document.getElementById('stat-gen').innerText = currentGen;
                document.getElementById('stat-best').innerText = latestBest.toLocaleString(undefined, {maximumFractionDigits: 1});
                document.getElementById('stat-median').innerText = latestMedian.toLocaleString(undefined, {maximumFractionDigits: 1});
                document.getElementById('stat-time').innerText = avgTime + ' s';
                
                updateChart(labels, bestData, medianData, worstData);
            } catch (e) {
                console.error("Error fetching log:", e);
            }
        }
        
        function updateChart(labels, best, median, worst) {
            if (!chart) {
                const ctx = document.getElementById('trainingChart').getContext('2d');
                Chart.defaults.color = '#94a3b8';
                Chart.defaults.font.family = "'Inter', sans-serif";
                
                chart = new Chart(ctx, {
                    type: 'line',
                    data: {
                        labels: labels,
                        datasets: [
                            {
                                label: 'Best Fitness',
                                data: best,
                                borderColor: '#10b981',
                                backgroundColor: 'rgba(16, 185, 129, 0.1)',
                                borderWidth: 3,
                                tension: 0.4,
                                fill: true
                            },
                            {
                                label: 'Median Fitness',
                                data: median,
                                borderColor: '#3b82f6',
                                borderWidth: 2,
                                borderDash: [5, 5],
                                tension: 0.4
                            },
                            {
                                label: 'Worst Fitness',
                                data: worst,
                                borderColor: '#ef4444',
                                borderWidth: 1,
                                tension: 0.4
                            }
                        ]
                    },
                    options: {
                        responsive: true,
                        maintainAspectRatio: false,
                        interaction: {
                            mode: 'index',
                            intersect: false,
                        },
                        plugins: {
                            legend: {
                                position: 'top',
                            },
                            tooltip: {
                                backgroundColor: 'rgba(15, 23, 42, 0.9)',
                                titleColor: '#f8fafc',
                                bodyColor: '#cbd5e1',
                                padding: 12,
                                cornerRadius: 8,
                                displayColors: true
                            }
                        },
                        scales: {
                            x: {
                                grid: {
                                    color: 'rgba(51, 65, 85, 0.5)'
                                }
                            },
                            y: {
                                grid: {
                                    color: 'rgba(51, 65, 85, 0.5)'
                                },
                                beginAtZero: true
                            }
                        }
                    }
                });
            } else {
                chart.data.labels = labels;
                chart.data.datasets[0].data = best;
                chart.data.datasets[1].data = median;
                chart.data.datasets[2].data = worst;
                chart.update();
            }
        }
        
        // Initial fetch and set interval
        fetchLog();
        setInterval(fetchLog, 5000);
    </script>
</body>
</html>
"""

class DashboardHandler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/' or self.path == '/index.html':
            self.send_response(200)
            self.send_header("Content-type", "text/html")
            self.end_headers()
            self.wfile.write(HTML_CONTENT.encode('utf-8'))
        elif self.path.startswith('/training_log.csv'):
            # Allow serving the CSV file
            log_path = os.path.join(DIRECTORY, 'training_log.csv')
            if os.path.exists(log_path):
                self.send_response(200)
                self.send_header("Content-type", "text/csv")
                self.send_header("Cache-Control", "no-cache, no-store, must-revalidate")
                self.end_headers()
                with open(log_path, 'rb') as f:
                    self.wfile.write(f.read())
            else:
                self.send_response(404)
                self.end_headers()
        else:
            self.send_response(404)
            self.end_headers()

def start_server():
    os.chdir(DIRECTORY)
    with socketserver.TCPServer(("", PORT), DashboardHandler) as httpd:
        print(f"Serving beautiful dashboard at http://localhost:{PORT}")
        httpd.serve_forever()

if __name__ == "__main__":
    # Start server in a background thread
    server_thread = threading.Thread(target=start_server, daemon=True)
    server_thread.start()
    
    # Open the browser automatically
    print("Opening dashboard in your web browser...")
    webbrowser.open(f"http://localhost:{PORT}")
    
    try:
        # Keep the main thread alive
        while True:
            import time
            time.sleep(1)
    except KeyboardInterrupt:
        print("Dashboard server stopped.")
