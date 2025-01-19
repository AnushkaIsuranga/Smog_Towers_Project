import React from "react";

const ContactSection: React.FC = () => {
  return (
    <div className="bg-green-700 text-white p-12 text-center">
      <h2 className="text-3xl font-semibold mb-4">Get in Touch</h2>
      <p className="text-lg mb-6">Have questions or suggestions? We’d love to hear from you.</p>
      <a href="mailto:info@envirosense.com" className="bg-white text-green-700 px-6 py-3 rounded-full text-lg shadow-lg hover:bg-green-100">
        Contact Us
      </a>
    </div>
  );
};

export default ContactSection;
