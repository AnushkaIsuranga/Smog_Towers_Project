import React, { useEffect, useState } from "react";
import { ref, onValue } from "firebase/database";
import { database } from "../config/firebaseConfig"; // Update this path if needed

interface DHTData {
  temperature: string;
  humidity: string;
}

const DHT11: React.FC = () => {
  const [dhtData, setDhtData] = useState<DHTData | null>(null);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    const dhtRef = ref(database, "sensorData"); // Updated the reference to match your structure

    onValue(dhtRef, (snapshot) => {
      const data = snapshot.val();
      console.log("Received data:", data); // Log the data for debugging
      if (data) {
        setDhtData({
          temperature: data.temperature ? data.temperature.toString() : "N/A",
          humidity: data.humidity ? data.humidity.toString() : "N/A",
        });
      } else {
        setDhtData({ temperature: "N/A", humidity: "N/A" });
      }
      setLoading(false);
    });
  }, []);

  if (loading) {
    return <div className="text-center text-lg">Loading sensor data...</div>;
  }

  if (!dhtData) {
    return <div className="text-center text-lg text-red-500">No data available.</div>;
  }

  return (
    <div className="bg-white shadow-md rounded-lg p-6 border border-gray-200">
      <h2 className="text-xl font-semibold text-gray-800">DHT Sensor Data</h2>
      <div className="mt-4 grid grid-cols-2">
        <p className="text-gray-700">
          <span className="font-bold">Temperature:</span> {dhtData.temperature}°C
        </p>
        <p className="text-gray-700">
          <span className="font-bold">Humidity:</span> {dhtData.humidity}%
        </p>
      </div>
    </div>
  );
};

export default DHT11;