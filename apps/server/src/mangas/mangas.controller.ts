import { Controller, Get, Post, Patch, Param, Query, Body, HttpCode, HttpStatus } from '@nestjs/common';
import { ApiTags, ApiOperation } from '@nestjs/swagger';
import { InjectModel } from '@nestjs/mongoose';
import { Model, Types } from 'mongoose';
import { Transform } from 'class-transformer';
import { IsOptional, IsNumber, IsString, IsArray } from 'class-validator';
import { Manga } from '../db/schemas/manga.schema';
import { Episode } from '../db/schemas/episode.schema';

class PaginationDto {
  @IsOptional()
  @IsNumber()
  @Transform(({ value }) => parseInt(value))
  page?: number;

  @IsOptional()
  @IsNumber()
  @Transform(({ value }) => parseInt(value))
  limit?: number;
}

class CreateMangaDto {
  @IsString()
  externalId: string;

  @IsString()
  title: string;

  @IsOptional()
  @IsString()
  coverImage?: string;

  @IsOptional()
  @IsString()
  source?: string;
}

class CreateEpisodeDto {
  @IsString()
  externalId: string;

  @IsString()
  title: string;

  @IsOptional()
  @IsNumber()
  number?: number;

  @IsOptional()
  @IsString()
  url?: string;

  @IsOptional()
  @IsArray()
  imageUrls?: string[];
}

class UpdateImagesDto {
  @IsArray()
  imageUrls: string[];
}

@ApiTags('Mangas')
@Controller('mangas')
export class MangasController {
  constructor(
    @InjectModel(Manga.name) private mangaModel: Model<Manga>,
    @InjectModel(Episode.name) private episodeModel: Model<Episode>,
  ) {}

  @Get()
  @ApiOperation({ summary: 'List all tracked mangas' })
  async findAll(@Query() query: PaginationDto) {
    const page = query.page || 1;
    const limit = query.limit || 20;
    const skip = (page - 1) * limit;
    const [mangas, total] = await Promise.all([
      this.mangaModel.find().skip(skip).limit(limit).sort({ updatedAt: -1 }),
      this.mangaModel.countDocuments(),
    ]);
    return { data: mangas, pagination: { page, limit, total, pages: Math.ceil(total / limit) } };
  }

  @Post()
  @HttpCode(HttpStatus.CREATED)
  @ApiOperation({ summary: 'Create or update a manga' })
  async create(@Body() dto: CreateMangaDto) {
    const existing = await this.mangaModel.findOne({ externalId: dto.externalId });
    
    if (existing) {
      Object.assign(existing, dto);
      await existing.save();
      return existing;
    }
    
    const manga = new this.mangaModel({
      ...dto,
      source: dto.source || 'asurascans',
      isBookmarked: true,
      lastCheckedAt: new Date(),
    });
    return manga.save();
  }

  @Get('external/:externalId')
  @ApiOperation({ summary: 'Get manga by external ID' })
  async findByExternalId(@Param('externalId') externalId: string) {
    const manga = await this.mangaModel.findOne({ externalId });
    if (!manga) return { error: 'Manga not found' };
    return manga;
  }

  @Get(':id')
  @ApiOperation({ summary: 'Get manga details with episodes' })
  async findOne(@Param('id') id: string) {
    const manga = await this.mangaModel.findById(id);
    if (!manga) return { error: 'Manga not found' };
    const episodes = await this.episodeModel.find({ mangaId: manga._id }).sort({ number: -1 });
    return { ...manga.toObject(), episodes };
  }

  @Get(':id/episodes')
  @ApiOperation({ summary: 'List episodes for a manga' })
  async findEpisodes(@Param('id') id: string, @Query() query: PaginationDto) {
    const page = query.page || 1;
    const limit = query.limit || 20;
    const skip = (page - 1) * limit;
    const mangaObjectId = new Types.ObjectId(id);
    const [episodes, total] = await Promise.all([
      this.episodeModel.find({ mangaId: mangaObjectId }).select('-embeddings').skip(skip).limit(limit).sort({ number: -1 }),
      this.episodeModel.countDocuments({ mangaId: mangaObjectId }),
    ]);
    return { data: episodes, pagination: { page, limit, total, pages: Math.ceil(total / limit) } };
  }

  @Post(':id/episodes')
  @HttpCode(HttpStatus.CREATED)
  @ApiOperation({ summary: 'Add episode to manga' })
  async createEpisode(@Param('id') id: string, @Body() dto: CreateEpisodeDto) {
    const manga = await this.mangaModel.findById(id);
    if (!manga) return { error: 'Manga not found' };

    const existing = await this.episodeModel.findOne({ 
      externalId: dto.externalId, 
      mangaId: manga._id 
    });
    
    if (existing) return existing;

    const episode = new this.episodeModel({
      mangaId: manga._id,
      externalId: dto.externalId,
      title: dto.title,
      number: dto.number || 0,
      url: dto.url || '',
      imageUrls: dto.imageUrls || [],
      isNew: true,
      crawledAt: new Date(),
    });
    return episode.save();
  }
}

@ApiTags('Episodes')
@Controller('episodes')
export class EpisodesController {
  constructor(@InjectModel(Episode.name) private episodeModel: Model<Episode>) {}

  @Get(':id/images')
  @ApiOperation({ summary: 'Get image URLs for an episode' })
  async getImages(@Param('id') id: string) {
    const episode = await this.episodeModel.findById(id).select('title number imageUrls url');
    if (!episode) return { error: 'Episode not found' };
    return episode;
  }

  @Patch(':id/images')
  @ApiOperation({ summary: 'Update episode images' })
  async updateImages(@Param('id') id: string, @Body() dto: UpdateImagesDto) {
    const episode = await this.episodeModel.findByIdAndUpdate(
      id,
      { imageUrls: dto.imageUrls, crawledAt: new Date(), isNew: false },
      { new: true }
    );
    if (!episode) return { error: 'Episode not found' };
    return episode;
  }

  @Get(':id')
  @ApiOperation({ summary: 'Get episode details' })
  async findOne(@Param('id') id: string) {
    return this.episodeModel.findById(id);
  }
}
