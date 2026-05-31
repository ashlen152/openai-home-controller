import { IsBoolean, IsNumber, IsOptional, IsString, Min, Max } from 'class-validator';

export class ReportPumpSettingsDto {
  @IsOptional()
  @IsString()
  pumpId?: string;

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
  @Min(1)
  @Max(288)
  scheduleSlots?: number;

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

  // Device health fields (from former POST /api/pump-status)
  @IsOptional()
  @IsBoolean()
  online?: boolean;

  @IsOptional()
  @IsNumber()
  lastHeartbeat?: number;

  @IsOptional()
  @IsNumber()
  wifiRssi?: number;

  @IsOptional()
  @IsString()
  ipAddress?: string;

  @IsOptional()
  @IsBoolean()
  isDosing?: boolean;

  @IsOptional()
  @IsNumber()
  totalDosedToday?: number;

  @IsOptional()
  @IsNumber()
  uptimeSeconds?: number;

  @IsOptional()
  @IsNumber()
  freeHeap?: number;

  @IsOptional()
  @IsNumber()
  lastSettingsSync?: number;
}
