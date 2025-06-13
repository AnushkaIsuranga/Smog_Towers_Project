Higher Diploma Final Project Submission
========================================

Project Title: Smog Tower Air Quality Monitoring and Forecasting System

Student Name: [Your Name Here]
Student ID: [Your Student ID Here]
Course: Higher Diploma in [Your Course Name]
Submission Date: 12 June 2025

Project Overview:
-----------------
This project presents a complete Air Quality Monitoring and Forecasting System, designed and implemented as part of my Higher Diploma final project. The system leverages IoT gas sensors, an ESP32 microcontroller, and a cloud-based web application to monitor, predict, and forecast Air Quality Index (AQI) in real time.

Key Features:
-------------
- **IoT Data Collection:** Gas sensors connected to an ESP32 microcontroller collect real-time air quality data (Ammonia, Benzene, CNG, CO, Hydrogen, LPG, Smoke, etc.).
- **Cloud Integration:** Sensor data is uploaded to Firebase Realtime Database for centralized storage and access.
- **Machine Learning Prediction:** A trained regression model (using scikit-learn) predicts AQI from sensor data.
- **Time Series Forecasting:** ARIMA model forecasts AQI for the next 3 days based on historical sensor readings.
- **Web Dashboard:** Flask-based web application displays current AQI, 3-day forecast, and visualizations for users and administrators.
- **User Interface:** Modern, responsive frontend for easy interaction and data visualization.

Project Structure:
------------------
- **/AQ/model/**: Jupyter notebooks, model training scripts, and saved ML models.
- **/AQ/website/**: Flask web application, templates, static files, and backend logic.
- **/ML/**: Datasets and additional notebooks for data analysis.
- **/Arduino code/** and **/esp32/**: Microcontroller code for sensor data acquisition and transmission.
- **/Web/**: Frontend (React/Tailwind) for extended UI (if applicable).

How to Run:
-----------
1. Ensure all dependencies are installed (see requirements.txt or environment setup in /AQ/website/env/).
2. Start the Flask server using the provided script or VS Code task.
3. Access the web dashboard at http://127.0.0.1:5000/.
4. Use the dashboard to view real-time AQI, predictions, and forecasts.

Examination Notes:
------------------
- All code, models, and documentation are original and developed by me for this project.
- The system demonstrates integration of IoT, cloud, machine learning, and web technologies.
- Please refer to the README.md files in each folder for detailed instructions and explanations.

Thank you for your consideration.
