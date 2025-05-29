from flask import Flask, render_template, request, jsonify
from flask_cors import CORS
import pickle
import numpy as np
import firebase_admin
from firebase_admin import credentials, db
import os

# Initialize the Flask app
app = Flask(__name__)
CORS(app, resources={r"/*": {"origins": "*"}}, supports_credentials=True)  # Enable CORS for all routes

# Load the saved model
with open(r'C:\Users\user\Desktop\AQ\website\model\predictor.pickle', 'rb') as file:
    model = pickle.load(file)

# Function to classify AQI
def get_aqi_status(aqi):
    if aqi <= 50:
        return "Good"
    elif aqi <= 100:
        return "Moderate"
    elif aqi <= 150:
        return "Unhealthy for Sensitive Groups"
    elif aqi <= 200:
        return "Unhealthy"
    elif aqi <= 300:
        return "Very Unhealthy"
    else:
        return "Deadly"

# Update Firebase config to use the provided database URL
FIREBASE_CONFIG = {
    'databaseURL': 'https://smog-tower-default-rtdb.asia-southeast1.firebasedatabase.app/'
}
FIREBASE_CRED_PATH = r'C:/Users/USER/Desktop/AQ/website/model/firebase-cred.json'  # Place your Firebase service account key here

firebase_initialized = False

def get_today_data_from_firebase():
    global firebase_initialized
    if not firebase_initialized:
        cred = credentials.Certificate('d:/new project/Smog_Towers_Project/Web/src/components/config/firebase-cred.json')
        firebase_admin.initialize_app(cred, {
            'databaseURL': 'https://smog-tower-default-rtdb.asia-southeast1.firebasedatabase.app/'
        })
        firebase_initialized = True
    ref = db.reference('gas_readings')
    data = ref.get()
    if not data:
        raise Exception('No data found in Firebase for gas_readings')
    keys = ['Ammonia','Benzene','Alcohol','CO','Methane','CNG','LPG','Hydrogen','Smoke','Propane']
    return {k: float(data.get(k, 0.0)) for k in keys}

# Route to display the HTML form
@app.route('/')
def index():
    return render_template('index.html')

# Route to handle form submission and prediction
@app.route('/predict', methods=['POST'])
def predict():
    try:
        print("Headers:", request.headers)  # Debug: Check headers
        print("Raw Data:", request.data)    # Debug: Raw request data
        print("Parsed JSON:", request.get_json())  # Debug: Parsed JSON
        
        if not request.is_json:
            return jsonify({"error": "Request must be JSON"}), 400
            
        form_data = request.get_json()
        print("Form Data:", form_data)  # Debug: Check received data
        
        input_data = np.array([[form_data.get(k, 0.0) for k in ['Ammonia','Benzene','Alcohol','CO','Methane','CNG','LPG','Hydrogen','Smoke','Propane']]])
        predictions = model.predict(input_data).flatten()
        
        days = ['Today', 'Day 2', 'Day 3', 'Day 4']
        results = [
            {
                'day': days[i],
                'aqi': float(predictions[i]),
                'status': get_aqi_status(predictions[i])
            } for i in range(len(predictions))
        ]
        return jsonify({'forecast': results})
    
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/forecast', methods=['GET'])
def get_forecast():
    try:
        # Always fetch latest sensor data from Firebase
        form_data = get_today_data_from_firebase()
        input_data = np.array([[form_data.get(k, 0.0) for k in ['Ammonia','Benzene','Alcohol','CO','Methane','CNG','LPG','Hydrogen','Smoke','Propane']]])
        predictions = model.predict(input_data).flatten()
        days = ['Today', 'Day 2', 'Day 3', 'Day 4']
        results = [
            {
                'day': days[i],
                'aqi': float(predictions[i]),
                'status': get_aqi_status(predictions[i])
            } for i in range(len(predictions))
        ]
        return jsonify({'forecast': results})
    except Exception as e:
        return jsonify({'error': str(e)}), 500

# Run the app
if __name__ == "__main__":
    app.run(debug=True)
