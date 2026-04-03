export interface MangaChapter {
  id: string;
  title: string;
  number: number;
  url: string;
  publishedAt: Date;
}

export interface MangaInfo {
  id: string;
  title: string;
  coverImage: string;
  description: string;
  chapters: MangaChapter[];
}

export interface ChapterImages {
  chapterId: string;
  images: string[];
}

export const MANGO_ADAPTER = 'MANGO_ADAPTER';

export interface IMangaAdapter {
  readonly name: string;
  readonly baseUrl: string;

  login(credentials: { email: string; password: string }): Promise<void>;
  isAuthenticated(): Promise<boolean>;

  getMangaInfo(mangaId: string): Promise<MangaInfo>;
  getBookmarks(): Promise<MangaInfo[]>;

  getChapterImages(chapterUrl: string): Promise<ChapterImages>;
  getLatestChapter(mangaId: string): Promise<MangaChapter | null>;
}
