import React from 'react'
import type { Manga } from '../../entities/manga/Manga'
// MangaCard is implemented inline to avoid cross-file import issues in this environment

type Props = {
  mangas: Manga[]
  onOpen: (manga: Manga) => void
}

export const MangaLibrary: React.FC<Props> = ({ mangas, onOpen }) => {
  const total = mangas.length
  const bookmarked = mangas.filter((m) => m.bookmarked).length
  const newChapters = mangas.reduce((acc, m) => acc + (m.newChapters ?? 0), 0)

  const onSync = () => {
    // Placeholder: In a full integration this would POST to /api/crawl/bookmarks/sync
    console.info('Sync bookmarks requested')
  }
  const onCheck = () => {
    // Placeholder: In a full integration this would GET /api/crawl/chapters/check
    console.info('Check new chapters requested')
  }
  return (
    <section>
      <header style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'baseline', marginBottom: 16 }}>
        <h2 style={{ margin: 0 }}>Manga Library</h2>
        <div style={{ display: 'flex', gap: 8, alignItems: 'center' }}>
          <span style={{ fontSize: 14, color: '#555' }}>
          Total: {total} • Bookmarked: {bookmarked} • New chapters: {newChapters}
          </span>
          <button onClick={onSync} style={{ padding: '6px 10px', borderRadius: 6, border: '1px solid #ddd', cursor: 'pointer' }}>Sync bookmarks</button>
          <button onClick={onCheck} style={{ padding: '6px 10px', borderRadius: 6, border: '1px solid #ddd', cursor: 'pointer' }}>Check new chapters</button>
        </div>
      </header>
      <div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fill, minmax(180px, 1fr))', gap: 16 }}>
        {mangas.map((m) => (
          <div
            key={m.id}
            onClick={() => onOpen(m)}
            style={{ cursor: 'pointer', border: '1px solid #e5e7eb', borderRadius: 8, overflow: 'hidden', background: '#fff' }}
          >
            <img src={m.coverUrl || 'https://via.placeholder.com/120x160?text=Manga'} alt={m.title}
                 style={{ width: '100%', height: 180, objectFit: 'cover' }} />
            <div style={{ padding: 8 }}>
              <div style={{ fontWeight: 600, fontSize: 14, lineHeight: 1.2 }}>{m.title}</div>
              <div style={{ fontSize: 12, color: '#666' }}>{m.source}</div>
            </div>
          </div>
        ))}
      </div>
    </section>
  )
}
