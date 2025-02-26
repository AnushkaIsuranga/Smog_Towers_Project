import { StrictMode } from 'react'
import { createRoot } from 'react-dom/client'
import './index.css'
import Home from './components/Home'
import SensorDashboard from './components/home/SensorDashboard'

createRoot(document.getElementById('root')!).render(
  <StrictMode>
    <Home/>
    {/* <SensorDashboard /> */}
  </StrictMode>,
)
