import { IsString, IsEnum, IsOptional, IsNumber, Min, Matches } from 'class-validator';

export class CreatePumpCommandDto {
  @IsString()
  @Matches(/^[a-zA-Z0-9_]{1,15}$/, {
    message: 'pumpId must be alphanumeric or underscore and max 15 characters',
  })
  pumpId: string;

  @IsEnum(['CALIBRATE', 'RESET', 'PAUSE', 'RESUME', 'TEST_DOSE', 'SAVE_CALIBRATION', 'SAVE_SETTINGS'])
  command: string;

  @IsOptional()
  @IsNumber()
  steps?: number;

  @IsOptional()
  @IsNumber()
  speed?: number;

  @IsOptional()
  payload?: Record<string, any>;

  @IsOptional()
  metadata?: Record<string, any>;
}
