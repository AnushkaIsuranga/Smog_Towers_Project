// src/firebaseConfig.ts
import { initializeApp } from "firebase/app";
import { getAnalytics } from "firebase/analytics";
import { getDatabase, ref, onValue } from "firebase/database";
import { getFirestore, collection, getDocs } from "firebase/firestore";

const firebaseConfig = {
    apiKey: "AIzaSyCKipnnsaq1vfaeCYmhuMI_mwaWSVVcP1M",
    authDomain: "smog-tower.firebaseapp.com",
    databaseURL: "https://smog-tower-default-rtdb.asia-southeast1.firebasedatabase.app",
    projectId: "smog-tower",
    storageBucket: "smog-tower.firebasestorage.app",
    messagingSenderId: "631981337065",
    appId: "1:631981337065:web:758f2daea5e6b2ab960378",
    measurementId: "G-HBG2HE3043"
};

const app = initializeApp(firebaseConfig);
const analytics = getAnalytics(app);
export const database = getDatabase(app);
export const firestore = getFirestore(app);
