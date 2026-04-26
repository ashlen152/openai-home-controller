// Manga entity type definitions
export type Episode = {
  id: string
  title: string
  chapter?: string
  date?: string
  url?: string
}

export type Manga = {
  id: string
  title: string
  source: string
  coverUrl?: string
  totalChapters?: number
  bookmarked?: boolean
  newChapters?: number
  episodes?: Episode[]
}

 
