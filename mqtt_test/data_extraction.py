from mqtt_client import mqtt_client
import numpy as np
from flask import Flask, render_template, jsonify
import json
from threading import Lock
from collections import deque

# Initialize Flask app
app = Flask(__name__)

# Global variables for storing sensor data
sensor_data = {
    'tof1': np.zeros((8, 8)),
    'tof2': np.zeros((8, 8)), 
}
data_lock = Lock()
history_buffer = deque(maxlen=100)  # Store last 100 measurements

def on_message(client, userdata, msg):
    try:
        # Parse the incoming data (assuming JSON format)
        data = json.loads(msg.payload.decode())
        data_array = np.array(data).reshape(8, 8)
        
        with data_lock:
            if msg.topic == mqtt_client.TOF1_TOPIC:
                sensor_data['tof1'] = data_array
            elif msg.topic == mqtt_client.TOF2_TOPIC:
                sensor_data['tof2'] = data_array
            
            # Create 3D point cloud from the two sensors
            combined_data = process_sensor_data()
            history_buffer.append(combined_data)
            
    except Exception as e:
        print(f"Error processing message: {e}")
        import traceback
        traceback.print_exc()  # This will print the full stack trace

def process_sensor_data():
    """Convert raw sensor data into 3D point cloud"""
    point_cloud = []
    
    for i in range(8):
        for j in range(8):
            # Sensor 1 data
            dist1 = float(sensor_data['tof1'][i][j])  # Convert to native Python float
            # Calculate x,y,z coordinates based on sensor position and readings
            x1 = float(i * 10)  # Convert to native Python float
            y1 = float(j * 10)
            z1 = dist1
            
            # Sensor 2 data
            dist2 = float(sensor_data['tof2'][i][j])  # Convert to native Python float
            x2 = float(i * 10)
            y2 = float(j * 10)
            z2 = dist2
            
            # Average the readings where they overlap
            point_cloud.append({
                'x': x1,
                'y': y1,
                'z': (z1 + z2) / 2 if z1 > 0 and z2 > 0 else max(z1, z2)
            })
            
    return point_cloud

@app.route('/')
def index():
    with data_lock:
        return render_template('visualization.html')

@app.route('/data')
def get_data():
    with data_lock:
        current_points = history_buffer[-1] if history_buffer else []
        return jsonify({
            'current': {
                'tof1': sensor_data['tof1'].tolist() if 'tof1' in sensor_data else [],
                'tof2': sensor_data['tof2'].tolist() if 'tof2' in sensor_data else [],
                'x': [p['x'] for p in current_points],
                'y': [p['y'] for p in current_points],
                'z': [p['z'] for p in current_points]
            },
            'history': list(history_buffer)
        })

if __name__ == '__main__':
    # Subscribe to both topics using the centralized client
    mqtt_client.subscribe([(mqtt_client.TOF1_TOPIC, 0), (mqtt_client.TOF2_TOPIC, 0)], on_message)
    app.run(host='0.0.0.0', port=5000)
