import type { Manga } from '../../entities/manga'
const BASE_PATH = '/api/mangas'

export async function fetchMangas(limit: number = 100): Promise<Manga[]> {
  const res = await fetch(`${BASE_PATH}?limit=${limit}`, {
    method: 'GET',
    headers: { 'Content-Type': 'application/json' },
  })
  if (!res.ok) {
    throw new Error(`Failed to fetch mangas: ${res.status}`)
  }
  const data = await res.json()
  // Expect an array of Manga objects; tolerate missing fields gracefully
  return Array.isArray(data) ? data as Manga[] : []
}

export async function fetchManga(id: string): Promise<Manga | null> {
  const res = await fetch(`${BASE_PATH}/${id}`, {
    method: 'GET',
    headers: { 'Content-Type': 'application/json' },
  })
  if (!res.ok) {
    // If not found, return null instead of throwing
    return null
  }
  const data = await res.json()
  return data as Manga
}
