import { IsString, IsNumber, Min, IsOptional, IsBoolean } from 'class-validator';

export class CalculateStepsDto {
  @IsString()
  pumpId: string;

  @IsNumber()
  @Min(1000)
  steps: number;

  @IsNumber()
  @Min(0.1)
  measuredML: number;

  @IsOptional()
  @IsBoolean()
  applyToPump?: boolean;
}
