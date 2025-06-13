from flask import Flask, render_template, request
import pickle
import numpy as np
import firebase_admin
from firebase_admin import credentials, db
import requests
import matplotlib.pyplot as plt
from statsmodels.tsa.arima.model import ARIMA

# Initialize the Flask app
app = Flask(__name__)

# Load the saved model
with open(r'D:\new project\Smog_Towers_Project\AQ\model\predictor.pickle', 'rb') as file:
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
FIREBASE_CRED_PATH = r'D:\new project\Smog_Towers_Project\Web\src\components\config\firebase-cred.json'  # Place your Firebase service account key here

firebase_initialized = False

def get_today_data_from_firebase():
    global firebase_initialized
    try:
        if not firebase_initialized:
            cred = credentials.Certificate(FIREBASE_CRED_PATH)
            firebase_admin.initialize_app(cred, FIREBASE_CONFIG)
            firebase_initialized = True
        ref = db.reference('gas_readings')
        data = ref.get()
        if data:
            # Only use the keys that are present in the database
            keys = ['Ammonia','Benzene','CNG','CO','Hydrogen','LPG','Smoke']
            result = {k: float(data.get(k, 0.0)) for k in keys}
            # Optionally, also fetch humidity and temperature if needed
            sensor_ref = db.reference('sensorData')
            sensor_data = sensor_ref.get()
            if sensor_data:
                result['humidity'] = float(sensor_data.get('humidity', 0.0))
                result['temperature'] = float(sensor_data.get('temperature', 0.0))
            return result
        else:
            raise ValueError("No data found in Firebase under 'gas_readings'.")
    except Exception as e:
        print(f"Firebase error: {e}")
        raise RuntimeError(f"Failed to fetch data from Firebase: {e}")

def get_historical_gas_data_from_firebase(limit=100):
    """
    Fetches the latest 'limit' gas readings from Firebase, ordered by timestamp/key.
    Assumes 'gas_readings' is a dict of dicts, each with gas sensor values.
    Returns a list of dicts sorted by key (timestamp or push id).
    Skips any entries that are not dicts.
    """
    global firebase_initialized
    try:
        if not firebase_initialized:
            cred = credentials.Certificate(FIREBASE_CRED_PATH)
            firebase_admin.initialize_app(cred, FIREBASE_CONFIG)
            firebase_initialized = True
        ref = db.reference('gas_readings')
        data = ref.get()
        if data:
            # Sort by key (assume key is timestamp or push id)
            sorted_items = sorted(data.items(), key=lambda x: x[0])
            # Only keep the last 'limit' records
            sorted_items = sorted_items[-limit:]
            records = []
            keys = ['Ammonia','Benzene','CNG','CO','Hydrogen','LPG','Smoke']
            for _, entry in sorted_items:
                if not isinstance(entry, dict):
                    continue  # skip non-dict entries
                record = {k: float(entry.get(k, 0.0)) for k in keys}
                records.append(record)
            return records
        else:
            raise ValueError("No data found in Firebase under 'gas_readings'.")
    except Exception as e:
        print(f"Firebase error: {e}")
        raise RuntimeError(f"Failed to fetch data from Firebase: {e}")

# Route to display the HTML form
@app.route('/')
def index():
    return render_template('index.html')

# Route to handle form submission and prediction
@app.route('/predict', methods=['POST'])
def predict():
    try:
        # Try to get today's data from Firebase, fallback to form or dummy
        if request.form.get('use_firebase'):
            form_data = get_today_data_from_firebase()
        else:
            form_data = {k: float(request.form[k]) for k in ['Ammonia','Benzene','CO','CNG','LPG','Hydrogen','Smoke']}
        input_data = np.array([[form_data['Ammonia'], form_data['Benzene'], form_data['CO'], form_data['CNG'], form_data['LPG'], form_data['Hydrogen'], form_data['Smoke']]])
        # Predict AQI for today
        today_aqi = model.predict(input_data).flatten()[0]
        results = [("Today", today_aqi, get_aqi_status(today_aqi))]

        # --- ARIMA Forecasting for next 3 days ---
        gas_records = get_historical_gas_data_from_firebase(limit=100)
        chart_data = None
        if gas_records and len(gas_records) >= 10:
            X_hist = np.array([[rec['Ammonia'], rec['Benzene'], rec['CNG'], rec['CO'], rec['LPG'], rec['Hydrogen'], rec['Smoke']] for rec in gas_records])
            aqi_series = model.predict(X_hist).flatten()
            # Fit ARIMA on all available data + today
            aqi_series_with_today = np.append(aqi_series, today_aqi)
            arima_model = ARIMA(aqi_series_with_today, order=(2,1,2))
            arima_fit = arima_model.fit()
            forecast_steps = 3
            arima_forecast = arima_fit.forecast(steps=forecast_steps)
            forecast_days = [f"Day {i+2}" for i in range(forecast_steps)]
            for i, aqi in enumerate(arima_forecast):
                results.append((forecast_days[i], aqi, get_aqi_status(aqi)))
            # Optionally, prepare chart_data for chart/table
            train_size = int(len(aqi_series_with_today) * 0.8)
            chart_data = {
                'train': aqi_series_with_today[:train_size].tolist(),
                'test': aqi_series_with_today[train_size:].tolist(),
                'forecast': arima_forecast.tolist(),
                'rmse': None,
                'forecast_table': [
                    {'index': len(aqi_series_with_today)+i, 'forecasted_aqi': float(aqi)}
                    for i, aqi in enumerate(arima_forecast)
                ]
            }
        return render_template('index.html', multi_predictions=results, chart_data=chart_data)
    except Exception as e:
        return render_template('index.html', prediction_text=f"Error: {e}")

@app.route('/forecast')
def forecast():
    try:
        # Fetch historical gas readings
        gas_records = get_historical_gas_data_from_firebase(limit=100)
        chart_data = None
        if gas_records and len(gas_records) >= 10:
            X_hist = np.array([[rec['Ammonia'], rec['Benzene'], rec['CNG'], rec['CO'], rec['LPG'], rec['Hydrogen'], rec['Smoke']] for rec in gas_records])
            aqi_series = model.predict(X_hist).flatten()
            train_size = int(len(aqi_series) * 0.8)
            train, test = aqi_series[:train_size], aqi_series[train_size:]
            arima_model = ARIMA(train, order=(2,1,2))
            arima_fit = arima_model.fit()
            forecast_steps = len(test)
            arima_forecast = arima_fit.forecast(steps=forecast_steps)
            rmse = np.sqrt(np.mean((test - arima_forecast) ** 2))
            # Prepare data for chart and table
            forecast_table = [
                {'index': i+train_size, 'forecasted_aqi': float(aqi)}
                for i, aqi in enumerate(arima_forecast)
            ]
            chart_data = {
                'train': train.tolist(),
                'test': test.tolist(),
                'forecast': arima_forecast.tolist(),
                'rmse': float(rmse),
                'forecast_table': forecast_table
            }
            return render_template('forecast.html', chart_data=chart_data)
        else:
            return render_template('forecast.html', prediction_text="Not enough data for forecasting.")
    except Exception as e:
        return render_template('forecast.html', prediction_text=f"Error: {e}")

# Run the app
if __name__ == "__main__":
    app.run(debug=True)