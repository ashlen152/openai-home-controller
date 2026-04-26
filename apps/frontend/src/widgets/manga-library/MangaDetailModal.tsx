import React from 'react'
import type { Manga } from '../../entities/manga/Manga'

type Props = {
  manga: Manga | null
  onClose: () => void
}

export const MangaDetailModal: React.FC<Props> = ({ manga, onClose }) => {
  if (!manga) return null
  const episodes = manga.episodes ?? []
  return (
    <div
      role="dialog"
      aria-label={`Manga details for ${manga.title}`}
      style={{ position: 'fixed', top: 0, left: 0, right: 0, bottom: 0, background: 'rgba(0,0,0,0.5)', display: 'flex', alignItems: 'center', justifyContent: 'center' }}
      onClick={onClose}
    >
      <div
        onClick={(e) => e.stopPropagation()}
        style={{ width: '90%', maxWidth: 700, background: '#fff', borderRadius: 8, padding: 16, maxHeight: '80%', overflow: 'auto' }}
      >
        <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: 8 }}>
          <h3 style={{ margin: 0 }}>{manga.title}</h3>
          <button onClick={onClose} style={{ border: 'none', background: 'transparent', fontSize: 18, cursor: 'pointer' }}>✕</button>
        </div>
        <h4 style={{ margin: '6px 0 12px' }}>Episodes</h4>
        <ul style={{ listStyle: 'none', padding: 0, margin: 0 }}>
          {episodes.map((ep) => (
            <li key={ep.id} style={{ padding: '6px 0', borderBottom: '1px solid #eee' }}>
              <span style={{ fontWeight: 500 }}>{ep.title}</span>
              {ep.chapter ? <span style={{ marginLeft: 8, color: '#666' }}>Ch {ep.chapter}</span> : null}
            </li>
          ))}
        </ul>
      </div>
    </div>
  )
}
