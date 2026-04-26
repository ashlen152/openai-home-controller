import { IsString, IsEnum, IsOptional, IsNumber } from 'class-validator';

export class CompletePumpCommandDto {
  @IsString()
  commandId: string;

  @IsEnum(['completed', 'failed', 'cancelled'])
  status: string;

  @IsOptional()
  @IsNumber()
  stepsCompleted?: number;

  @IsOptional()
  @IsString()
  error?: string;
}
