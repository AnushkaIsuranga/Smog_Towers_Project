import React from "react";
import HeroSection from "./home/HeroSection";
import AboutSection from "./home/AboutSection";
import SensorsSection from "./home/SensorDashboard";
import FurtherDetailsSection from "./home/FurtherDetailsSection";
import ContactSection from "./home/ContactSection";

const Home: React.FC = () => {
  return (
    <div className="min-h-screen bg-green-50 text-gray-800">
      <HeroSection />
      <AboutSection />
      <SensorsSection />
      <FurtherDetailsSection />
      <ContactSection />
    </div>
  );
};

export default Home;
