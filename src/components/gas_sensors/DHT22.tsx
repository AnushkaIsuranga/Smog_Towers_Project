import React, { useEffect, useState } from "react";
import { ref, onValue } from "firebase/database";
import { database } from "../config/firebaseConfig"; // Update this path to match your Firebase config

interface DHTData {
  temperature: string;
  humidity: string;
}

const DHT11: React.FC = () => {
  const [dhtData, setDhtData] = useState<DHTData | null>(null);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    const dhtRef = ref(database, "DHT"); // Update the path if needed

    onValue(dhtRef, (snapshot) => {
      const data = snapshot.val();
      console.log("Received data:", data); // Log the data to the console for debugging
      if (data) {
        // Check if the keys 'temperature' and 'humidity' exist
        setDhtData({
          temperature: data.Temperature ? data.Temperature.toString() : "N/A",
          humidity: data.Humidity ? data.Humidity.toString() : "N/A",
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
