import React from "react";
import Cover from "../assets/image.png";

const HeroSection: React.FC = () => {
  return (
    <div className="text-center w-full h-screen p-12">
        <div id="Hero" className="bg-green-700 h-full text-white flex flex-col items-center justify-center ">
            <h1 className="text-5xl font-bold mb-4">Welcome to EnviroSense</h1><br />
            <p className="text-xl mb-6">
                Your gateway to monitoring real-time environmental data and making eco-friendly decisions.
            </p>
            <button className="bg-green-500 px-6 py-3 rounded-full text-white text-lg shadow-lg transform duration-300 transition-all hover:bg-white hover:text-green-600 hover:font-bold">
                Get Started
            </button>
        </div>
    </div>
  );
};

export default HeroSection;
