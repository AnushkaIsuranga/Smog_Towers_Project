import React, { useEffect, useState } from "react";
import { ref, onValue } from "firebase/database";
import { database } from "../config/firebaseConfig";
import DHT22 from "../gas_sensors/DHT22";

const SensorDashboard: React.FC = () => {
  const [sensorData, setSensorData] = useState<{ [key: string]: number }>({});
  const [loading, setLoading] = useState(true);
  const [dataLog, setDataLog] = useState<Array<{ timestamp: string; data: { [key: string]: number } }>>([]);

  useEffect(() => {
    const gasRef = ref(database, "gas_readings");

    onValue(gasRef, (snapshot) => {
      const data = snapshot.val();
      if (data) {
        setSensorData(data);

        // Log the data with a timestamp
        const timestamp = new Date().toISOString();
        setDataLog((prevLog) => [...prevLog, { timestamp, data }]);
      }
      setLoading(false);
    });
  }, []);

  const exportToCSV = () => {
    // Create CSV headers
    const headers = ["Timestamp", ...Object.keys(sensorData)].join(",");

    // Create CSV rows
    const rows = dataLog.map((entry) => {
      const values = Object.values(entry.data).map((value) => value.toFixed(5));
      return [entry.timestamp, ...values].join(",");
    });

    // Combine headers and rows
    const csvContent = [headers, ...rows].join("\n");

    // Create a Blob and trigger download
    const blob = new Blob([csvContent], { type: "text/csv" });
    const url = URL.createObjectURL(blob);
    const link = document.createElement("a");
    link.href = url;
    link.download = "gas_data.csv";
    link.click();
    URL.revokeObjectURL(url);
  };

  if (loading) {
    return <div className="text-center text-lg">Loading sensor data...</div>;
  }

  return (
    <div className="p-4">
      <div className="mb-8">
        <h1 className="text-2xl font-bold mb-4">DHT22 - Environmental Sensor</h1>
        <p className="text-gray-700 mb-4">Measures Temperature and Humidity</p>
        <DHT22 />
      </div>
      <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
        {Object.entries(sensorData).map(([gas, value]) => (
          <div key={gas} className="bg-white shadow-md rounded-lg p-4 border border-gray-200">
            <h2 className="text-xl font-semibold text-gray-800">{gas}</h2>
            <p className="text-gray-700 mt-2">
              <span className="font-bold">PPM Units:</span> {value.toFixed(5)}
            </p>
          </div>
        ))}
      </div>
      <button
        onClick={exportToCSV}
        className="mt-4 bg-blue-500 text-white px-4 py-2 rounded justify-center hover:bg-blue-600"
      >
        Export Data to CSV
      </button>
    </div>
  );
};

export default SensorDashboard;