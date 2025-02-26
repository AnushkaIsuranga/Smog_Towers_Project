// import React, { useState, useEffect } from "react";
// import { ref, onValue } from "firebase/database";
// import { database } from "./firebaseConfig";

// interface GasData {
//   gasName: string;
//   ppm: string;
//   level: string;
// }

// export const useGasData = (path: string): GasData[] => {
//   const [gasData, setGasData] = useState<GasData[]>([]);

//   useEffect(() => {
//     const dbRef = ref(database, path);
//     const unsubscribe = onValue(dbRef, (snapshot) => {
//       const data = snapshot.val();
//       const formattedData: GasData[] = [];

//       // Loop through the database to extract the ppm and levels
//       Object.keys(data).forEach((sensor) => {
//         const sensorData = data[sensor];
//         Object.keys(sensorData).forEach((gas) => {
//           const gasInfo = sensorData[gas];
//           const [ppm, level] = gasInfo.match(/([\d.]+) ppm \((\w+)\)/)?.slice(1) || [];
//           if (ppm && level) {
//             formattedData.push({ gasName: gas, ppm, level });
//           }
//         });
//       });

//       setGasData(formattedData);
//     });

//     return () => unsubscribe();
//   }, [path]);

//   return gasData;
// };
