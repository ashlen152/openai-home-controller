import { IsBoolean, IsNumber, IsString, IsEnum, Min, Matches, ValidateNested, IsOptional } from 'class-validator';
import { Type } from 'class-transformer';
import { DoseEventStatus } from '../schemas/dose-event.schema';
import { MetadataDto } from './metadata.dto';

export class CreateDoseEventDto {
  @IsString()
  @Matches(/^[a-zA-Z0-9_]{1,15}$/, {
    message: 'pumpId must be alphanumeric or underscore and max 15 characters',
  })
  pumpId: string;

  @IsString()
  eventId: string;

  @IsNumber()
  timestamp: number;

  @IsNumber()
  @Min(0.01, { message: 'Volume must be greater than 0' })
  volume: number;

  @IsEnum(DoseEventStatus)
  status: string;

  @IsOptional()
  @IsBoolean()
  success?: boolean | null;

  @ValidateNested()
  @Type(() => MetadataDto)
  metadata: MetadataDto;
}
