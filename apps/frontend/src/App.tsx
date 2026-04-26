import React, { useState } from 'react'
import PumpControllerPage from './pages/pump-controller/index'
import MangaCrawlerPage from './pages/manga-crawler/MangaCrawlerPage'

type Page = 'pump' | 'manga'

const App: React.FC = () => {
  const [currentPage, setCurrentPage] = useState<Page>('pump')
  
  return (
    <div style={{ minHeight: '100vh', margin: 0 }}>
      <header style={{
        display: 'flex',
        alignItems: 'center',
        justifyContent: 'space-between',
        padding: '12px 20px',
        backgroundColor: '#1a1a2e',
        color: '#fff',
        boxShadow: '0 2px 8px rgba(0,0,0,0.15)'
      }}>
        <div style={{ display: 'flex', alignItems: 'center', gap: 8 }}>
          <span style={{ fontSize: 20 }}>⚡</span>
          <span style={{ fontWeight: 600, fontSize: 18 }}>OpenAI Workflow</span>
        </div>
        <nav style={{ display: 'flex', gap: 8 }}>
          <button
            onClick={() => setCurrentPage('pump')}
            style={{
              padding: '8px 16px',
              borderRadius: 6,
              border: 'none',
              cursor: 'pointer',
              fontWeight: 500,
              backgroundColor: currentPage === 'pump' ? '#4a90d9' : '#2d2d44',
              color: '#fff',
              transition: 'background-color 0.2s'
            }}
          >
            💧 Pump Controller
          </button>
          <button
            onClick={() => setCurrentPage('manga')}
            style={{
              padding: '8px 16px',
              borderRadius: 6,
              border: 'none',
              cursor: 'pointer',
              fontWeight: 500,
              backgroundColor: currentPage === 'manga' ? '#4a90d9' : '#2d2d44',
              color: '#fff',
              transition: 'background-color 0.2s'
            }}
          >
            📚 Manga Crawler
          </button>
        </nav>
      </header>
      
      <main style={{ padding: 20 }}>
        {currentPage === 'pump' && <PumpControllerPage />}
        {currentPage === 'manga' && <MangaCrawlerPage />}
      </main>
    </div>
  )
}

export default App
