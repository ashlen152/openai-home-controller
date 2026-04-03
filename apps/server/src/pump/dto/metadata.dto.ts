import { IsBoolean, IsNumber } from 'class-validator';

export class MetadataDto {
  @IsNumber()
  totalToday: number;

  @IsNumber()
  remaining: number;

  @IsBoolean()
  isAuto: boolean;
}
