import json
import socket
import hashlib
import struct
import csv
import os
from flask import Flask, render_template, jsonify, request
from threading import Thread, Lock
import time
from collections import deque

# Inisialisasi Flask
app = Flask(__name__)

# Variabel global
log_lock = Lock()
logs_file = "mining_logs.csv"
log_buffer = deque(maxlen=100)  # Buffer log untuk tampilan real-time
mining_active = False
hash_rate = 0
jobs_processed = 0
current_job_id = None

# Membaca konfigurasi dari config.json
def load_config():
    try:
        with open('config.json', 'r') as f:
            return json.load(f)
    except FileNotFoundError:
        return None

# Menyimpan log ke file CSV
def save_log_to_csv(log_entry):
    with log_lock:
        file_exists = os.path.isfile(logs_file)
        with open(logs_file, mode='a', newline='') as f:
            writer = csv.writer(f)
            if not file_exists:
                writer.writerow(["timestamp", "job_id", "hash_rate", "nonce", "status", "hash"])  # Header
            writer.writerow(log_entry)

# Membuat koneksi ke pool
def connect_to_pool(config):
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((config['pool']['url'], config['pool']['port']))
        return sock
    except Exception as e:
        log_buffer.append(f"Error connecting to pool: {e}")
        return None

# Mengirim data ke pool
def send_data(sock, data):
    try:
        sock.sendall(data.encode('utf-8'))
        response = sock.recv(4096).decode('utf-8')
        return response
    except Exception as e:
        log_buffer.append(f"Error sending data: {e}")
        return None

# Menghitung hash blok dan menyiapkan pekerjaan
def process_block(block_data, difficulty):
    nonce = 0
    block_data_bytes = block_data.encode('utf-8')
    while True:
        block_data_with_nonce = block_data_bytes + struct.pack("<I", nonce)
        hash_result = hashlib.sha256(block_data_with_nonce).hexdigest()
        if int(hash_result, 16) < difficulty:
            return nonce, hash_result
        nonce += 1

# Program mining
def mine():
    global mining_active, hash_rate, jobs_processed, current_job_id

    config = load_config()
    if not config:
        log_buffer.append("No configuration found.")
        return

    sock = connect_to_pool(config)
    if not sock:
        log_buffer.append("Failed to connect to pool.")
        return

    try:
        log_buffer.append("Connected to pool. Starting mining...")
        mining_active = True
        difficulty = int(config['mining']['difficulty'], 16)

        while mining_active:
            job_id = f"Job_{jobs_processed + 1}"  # Simulasi job ID
            block_data = f"block_data_{job_id}"  # Simulasi block data
            current_job_id = job_id

            start_time = time.time()
            nonce, hash_result = process_block(block_data, difficulty)
            end_time = time.time()

            duration = end_time - start_time
            hash_rate = int(1 / duration)  # Hash rate dalam hash/detik
            jobs_processed += 1

            log_entry = [
                time.strftime("%Y-%m-%d %H:%M:%S"),
                job_id,
                hash_rate,
                nonce,
                "Success",
                hash_result,
            ]
            save_log_to_csv(log_entry)
            log_buffer.append(f"Job {job_id}: Nonce {nonce}, Hash {hash_result[:10]}..., Rate {hash_rate} H/s")
            time.sleep(1)  # Simulasi delay antar pekerjaan
    finally:
        sock.close()
        mining_active = False
        log_buffer.append("Mining stopped.")

# Endpoint untuk memulai mining
@app.route('/start_mining', methods=['POST'])
def start_mining():
    global mining_active
    if not mining_active:
        mining_thread = Thread(target=mine)
        mining_thread.daemon = True
        mining_thread.start()
        return jsonify({"status": "Mining started"})
    return jsonify({"status": "Mining is already active"})

# Endpoint untuk menghentikan mining
@app.route('/stop_mining', methods=['POST'])
def stop_mining():
    global mining_active
    mining_active = False
    return jsonify({"status": "Mining stopped"})

# Endpoint untuk status mining
@app.route('/status', methods=['GET'])
def status():
    return jsonify({
        "mining_active": mining_active,
        "hash_rate": hash_rate,
        "jobs_processed": jobs_processed,
        "current_job_id": current_job_id,
        "log": list(log_buffer),
    })

# Halaman utama dengan dashboard
@app.route('/')
def index():
    return render_template('index.html')

# Halaman untuk pengaturan
@app.route('/settings', methods=['GET', 'POST'])
def settings():
    if request.method == 'POST':
        pool_url = request.form.get('pool_url')
        pool_port = int(request.form.get('pool_port'))
        worker_user = request.form.get('worker_user')
        worker_pass = request.form.get('worker_pass', 'x')
        wallet_address = request.form.get('wallet_address')

        config = {
            "pool": {
                "url": pool_url,
                "port": pool_port,
                "user": worker_user,
                "pass": worker_pass
            },
            "mining": {
                "difficulty": "0000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
            },
            "wallet_address": wallet_address
        }

        with open('config.json', 'w') as f:
            json.dump(config, f)

        return render_template('index.html', message="Settings saved.")
    return render_template('settings.html')

if __name__ == "__main__":
    app.run(debug=True, use_reloader=False)
