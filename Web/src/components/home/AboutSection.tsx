import React from "react";

const AboutSection: React.FC = () => {
  return (
    <div className="p-12 bg-white text-center">
      <h2 className="text-3xl font-semibold text-green-800 mb-4">About EnviroSense</h2>
      <p className="text-lg text-gray-600">
        EnviroSense provides real-time data from a variety of sensors to monitor and assess the health of our planet.
        With an easy-to-use interface, we empower you to make informed, eco-friendly decisions that help protect the environment.
      </p>
    </div>
  );
};

export default AboutSection;
