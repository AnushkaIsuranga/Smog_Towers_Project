import React, { useEffect, useState } from "react";
import { ref, onValue } from "firebase/database";
import { database } from "../config/firebaseConfig";
import DHT22 from "../gas_sensors/DHT22";

interface Gas {
  name: string;
  ppm: string;
  level: string;
}

const thresholds: { [key: string]: { low: number; moderate: number; high: number } } = {
  "CNG": { low: 300, moderate: 1000, high: 2000 },
  "Methane (CH₄)": { low: 500, moderate: 1500, high: 2500 },
  "LPG": { low: 800, moderate: 2000, high: 4000 },
  "Butane (C₄H₁₀)": { low: 400, moderate: 1200, high: 2000 },
  "Hydrogen (H₂)": { low: 200, moderate: 800, high: 1500 },
  "Smoke": { low: 500, moderate: 1500, high: 2500 },
  "Alcohol": { low: 1000, moderate: 2000, high: 3000 },
  "Propane (C₃H₈)": { low: 200, moderate: 500, high: 800 },
  "CarbonMonoxide (CO)": { low: 200, moderate: 500, high: 800 },
  "Ammonia (NH₃)": { low: 50, moderate: 200, high: 400 },
  "Benzene (C₆H₆)": { low: 50, moderate: 100, high: 150 },
};

const SensorDashboard: React.FC = () => {
  const [sensorData, setSensorData] = useState<{ [key: string]: Gas[] }>({});
  const [loading, setLoading] = useState(true);

  const sensors = [
    { id: "MQ4", type: "Methane Sensor", description: "Detects Methane, Natural Gas, CNG" },
    { id: "C6H6", type: "Gas Sensor", description: "Detects LPG, Butane, Benzene, Smoke" },
    { id: "MQ7", type: "Carbon Monoxide Sensor", description: "Detects Carbon Monoxide, Hydrogen" },
    { id: "MQ8", type: "Hydrogen Gas Sensor", description: "Detects Hydrogen" },
    { id: "MQ2", type: "Gas Sensor", description: "Detects LPG, Methane, Smoke, Hydrogen, Propane" },
    { id: "MQ135", type: "Air Quality Sensor", description: "Detects Ammonia, CO2, Benzene, Alcohol, Smoke, Sulfide" },
  ];

  useEffect(() => {
    const fetchData = async () => {
      const data: { [key: string]: Gas[] } = {};
      let isLoading = true;

      for (const sensor of sensors) {
        const gasRef = ref(database, sensor.id);
        await new Promise((resolve) => {
          onValue(gasRef, (snapshot) => {
            const rawData = snapshot.val();
            if (rawData) {
              const parsedData: Gas[] = Object.keys(rawData).map((gasKey) => {
                const match = rawData[gasKey]?.match(/(.*?):\s([\d.]+)ppm\s+\((.*?)\)/);
                if (match) {
                  return {
                    name: match[1],
                    ppm: match[2],
                    level: match[3],
                  };
                }
                return { name: gasKey, ppm: "N/A", level: "N/A" };
              });
              data[sensor.id] = parsedData;
            } else {
              data[sensor.id] = [];
            }
            resolve(null);
          });
        });
      }

      setSensorData(data);
      isLoading = false;
      setLoading(isLoading);
    };

    fetchData();
  }, [sensors]);

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
      <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
        {sensors.map((sensor) => (
          <div key={sensor.id} className="mb-8">
            <h1 className="text-2xl font-bold mb-4">{sensor.id} - {sensor.type}</h1>
            <p className="text-gray-700 mb-4">{sensor.description}</p>
            {sensorData[sensor.id]?.length ? (
              <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                {sensorData[sensor.id].map((gas, index) => {
                  const threshold = thresholds[gas.name];

                  return (
                    <div key={index} className="bg-white shadow-md rounded-lg p-4 border border-gray-200">
                      <h2 className="text-xl font-semibold text-gray-800">{gas.name}</h2>
                      <p className="text-gray-700 mt-2">
                        <span className="font-bold">PPM Units:</span> {gas.ppm}
                      </p>
                      <p className="text-gray-700">
                        <span className="font-bold">Level:</span> {gas.level}
                      </p>
                      <p className="text-gray-700 mt-2">
                        <span className="font-bold">Low Threshold:</span> {threshold?.low} PPM
                      </p>
                      <p className="text-gray-700">
                        <span className="font-bold">Moderate Threshold:</span> {threshold?.moderate} PPM
                      </p>
                      <p className="text-gray-700">
                        <span className="font-bold">High Threshold:</span> {threshold?.high} PPM
                      </p>
                    </div>
                  );
                })}
              </div>
            ) : (
              <div className="text-center text-lg text-red-500">No data available.</div>
            )}
          </div>
        ))}
      </div>
    </div>
  );
};

export default SensorDashboard;
