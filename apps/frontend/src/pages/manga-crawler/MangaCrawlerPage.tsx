import React, { useEffect, useState } from 'react'
import { fetchMangas } from '../../shared/api/manga'
import { MangaLibrary } from '../../widgets/manga-library'
import type { Manga } from '../../entities/manga/Manga'
import { MangaDetailModal } from '../../widgets/manga-library'

const MangaCrawlerPage: React.FC = () => {
  const [mangas, setMangas] = useState<Manga[]>([])
  const [selected, setSelected] = useState<Manga | null>(null)
  const [loading, setLoading] = useState<boolean>(true)

  useEffect(() => {
    let mounted = true
    fetchMangas(100)
      .then((list) => {
        if (mounted) {
          setMangas(list)
          setLoading(false)
        }
      })
      .catch(() => {
        if (mounted) setLoading(false)
      })
    return () => {
      mounted = false
    }
  }, [])

  return (
    <div style={{ padding: 16 }}>
      {loading ? <div>Loading mangas…</div> : (
        <MangaLibrary mangas={mangas} onOpen={setSelected} />
      )}
      <MangaDetailModal manga={selected} onClose={() => setSelected(null)} />
    </div>
  )
}

export default MangaCrawlerPage
