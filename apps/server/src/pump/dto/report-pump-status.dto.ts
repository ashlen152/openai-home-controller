import { IsBoolean, IsNumber, IsOptional, IsString } from 'class-validator';

export class ReportPumpStatusDto {
  @IsString()
  @IsOptional()
  pumpId?: string;

  @IsBoolean()
  @IsOptional()
  online?: boolean;

  @IsNumber()
  @IsOptional()
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
  @IsBoolean()
  reportedEnabled?: boolean;

  @IsOptional()
  @IsNumber()
  reportedDailyVolume?: number;

  @IsOptional()
  @IsNumber()
  reportedDayStartHour?: number;

  @IsOptional()
  @IsNumber()
  reportedDayEndHour?: number;

  @IsOptional()
  @IsNumber()
  reportedDayPercent?: number;

  @IsOptional()
  @IsNumber()
  reportedScheduleSlots?: number;

  @IsOptional()
  @IsNumber()
  reportedStepsPerML?: number;

  @IsOptional()
  @IsNumber()
  reportedActiveProfile?: number;

  @IsOptional()
  @IsNumber()
  reportedPausedUntil?: number;

  @IsOptional()
  @IsNumber()
  lastSettingsSync?: number;

  @IsOptional()
  @IsNumber()
  uptimeSeconds?: number;

  @IsOptional()
  @IsNumber()
  freeHeap?: number;
}
