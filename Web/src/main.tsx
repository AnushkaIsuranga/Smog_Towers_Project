import { StrictMode } from 'react'
import { createRoot } from 'react-dom/client'
import './index.css'
import Home from './components/Home'
import { Analytics } from "@vercel/analytics/react"

createRoot(document.getElementById('root')!).render(
  <StrictMode>
    <Home/>
    <Analytics/>
  </StrictMode>,
)
