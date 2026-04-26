import React from 'react'
import type { Manga } from '../../entities/manga/Manga'

type Props = {
  mangas: Manga[]
}

export const MangaList: React.FC<Props> = ({ mangas }) => {
  return (
    <ul style={{ listStyle: 'none', padding: 0, margin: 0 }}>
      {mangas.map((m) => (
        <li key={m.id} style={{ padding: '8px 0', borderBottom: '1px solid #eee' }}>
          <span style={{ fontWeight: 600 }}>{m.title}</span>
          <span style={{ marginLeft: 8, color: '#666' }}>{m.source}</span>
        </li>
      ))}
    </ul>
  )
}
