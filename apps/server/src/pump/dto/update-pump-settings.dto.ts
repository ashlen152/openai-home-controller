import {
  IsBoolean,
  IsNumber,
  IsString,
  IsOptional,
  Min,
  Max,
  Matches,
} from 'class-validator';

export class UpdatePumpSettingsDto {
  @IsString()
  @Matches(/^[a-zA-Z0-9_]{1,15}$/, {
    message: 'pumpId must be alphanumeric or underscore and max 15 characters',
  })
  pumpId: string;

  @IsOptional()
  @IsBoolean()
  enabled?: boolean;

  @IsOptional()
  @IsNumber()
  dailyVolume?: number;

  @IsOptional()
  @IsNumber()
  @Min(0)
  @Max(23)
  dayStartHour?: number;

  @IsOptional()
  @IsNumber()
  @Min(0)
  @Max(23)
  dayEndHour?: number;

  @IsOptional()
  @IsNumber()
  @Min(0)
  @Max(100)
  dayPercent?: number;

  @IsOptional()
  @IsNumber()
  stepsPerML?: number;

  @IsOptional()
  @IsNumber()
  @Min(0)
  activeProfile?: number;

  @IsOptional()
  @IsNumber()
  @Min(0)
  pausedUntil?: number;
}
