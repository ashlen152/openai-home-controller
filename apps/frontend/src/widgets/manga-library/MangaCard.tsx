import React from 'react'
import type { Manga } from '../../entities/manga/Manga'

type Props = {
  manga: Manga
  onClick?: () => void
}

export const MangaCard: React.FC<Props> = ({ manga, onClick }) => {
  const cover = manga.coverUrl || 'https://via.placeholder.com/120x160?text=Manga'
  return (
    <div
      onClick={onClick}
      style={{ cursor: 'pointer', border: '1px solid #e5e7eb', borderRadius: 8, overflow: 'hidden', background: '#fff' }}
    >
      <img src={cover} alt={manga.title} style={{ width: '100%', height: 180, objectFit: 'cover' }} />
      <div style={{ padding: 8 }}>
        <div style={{ fontWeight: 600, fontSize: 14, lineHeight: 1.2 }}>{manga.title}</div>
        <div style={{ fontSize: 12, color: '#666' }}>{manga.source}</div>
      </div>
    </div>
  )
}
