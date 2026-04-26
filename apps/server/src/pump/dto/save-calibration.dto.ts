import { IsString, IsOptional, IsNumber } from 'class-validator';

export class SaveCalibrationDto {
  @IsNumber()
  measuredML: number;
}

export class SaveCalibrationCommandDto {
  @IsString()
  pumpId: string;

  @IsString()
  command: string;

  @IsOptional()
  @IsNumber()
  steps?: number;

  @IsOptional()
  @IsNumber()
  speed?: number;

  @IsOptional()
  metadata?: Record<string, any>;
}
