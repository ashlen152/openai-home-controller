import { IsString, IsNumber, Min, Max } from 'class-validator';

export class CalibrateCommandDto {
  @IsString()
  pumpId: string;

  @IsNumber()
  @Min(1)
  @Max(50)
  volume?: number = 5.0;
}

export class TestDoseCommandDto {
  @IsString()
  pumpId: string;

  @IsNumber()
  @Min(0.1)
  @Max(100)
  volume: number;

  @IsNumber()
  @Min(500)
  @Max(10000)
  speed?: number;
}

export class SaveCalibrationCommandDto {
  @IsString()
  pumpId: string;

  @IsNumber()
  @Min(0.1)
  @Max(100)
  measuredML: number;
}
