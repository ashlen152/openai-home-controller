import { Controller, Get, Post, Body, Param, NotFoundException, Delete } from '@nestjs/common';
import { PumpSettingsService } from '../services/pump-settings.service';
import { PumpStatusService } from '../services/pump-status.service';
import { PumpCommandsService } from '../services/pump-commands.service';
import { CreatePumpSettingsDto } from '../dto/create-pump-settings.dto';
import { UpdatePumpSettingsDto } from '../dto/update-pump-settings.dto';
import { ReportPumpSettingsDto } from '../dto/report-pump-settings.dto';
import { PumpSetting } from '../schemas/pump-setting.schema';

@Controller('pump-settings')
export class PumpSettingsController {
  constructor(
    private readonly pumpSettingsService: PumpSettingsService,
    private readonly pumpStatusService: PumpStatusService,
    private readonly pumpCommandsService: PumpCommandsService,
  ) {}

  @Get(':pumpId')
  async getSettings(@Param('pumpId') pumpId: string): Promise<PumpSetting> {
    const settings = await this.pumpSettingsService.getSettings(pumpId);
    if (!settings) {
      throw new NotFoundException(`Pump settings not found for pumpId: ${pumpId}`);
    }
    return settings;
  }

  @Get()
  async getAllPumps(): Promise<any[]> {
    const pumps = await this.pumpSettingsService.getAllPumps();
    const statuses = await this.pumpStatusService.getAllStatus();
    const statusMap = new Map(statuses.map((s) => [s.pumpId, s]));

    return pumps.map((p) => {
      const status = statusMap.get(p.pumpId);
      const now = Date.now();
      const isOnline = status?.online && now - (status.lastHeartbeat * 1000 || 0) < 120_000;

      const serverSettings = {
        enabled: p.enabled,
        dailyVolume: p.dailyVolume,
        scheduleSlots: p.scheduleSlots,
        dayStartHour: p.dayStartHour,
        dayEndHour: p.dayEndHour,
        dayPercent: p.dayPercent,
        stepsPerML: p.stepsPerML,
        activeProfile: p.activeProfile,
        pausedUntil: p.pausedUntil,
      };

      const reportedSettings = status
        ? {
            enabled: status.reportedEnabled,
            dailyVolume: status.reportedDailyVolume,
            scheduleSlots: status.reportedScheduleSlots,
            dayStartHour: status.reportedDayStartHour,
            dayEndHour: status.reportedDayEndHour,
            dayPercent: status.reportedDayPercent,
            stepsPerML: status.reportedStepsPerML,
            activeProfile: status.reportedActiveProfile,
            pausedUntil: status.reportedPausedUntil,
          }
        : null;

      const mismatches = reportedSettings
        ? this.findMismatches(serverSettings, reportedSettings)
        : [];

      const doc = p as any;
      return {
        pumpId: p.pumpId,
        enabled: p.enabled,
        dailyVolume: p.dailyVolume,
        scheduleSlots: p.scheduleSlots,
        dayStartHour: p.dayStartHour,
        dayEndHour: p.dayEndHour,
        dayPercent: p.dayPercent,
        stepsPerML: p.stepsPerML,
        activeProfile: p.activeProfile,
        pausedUntil: p.pausedUntil,
        lastSync: doc.lastSync,
        calibrationHistory: doc.calibrationHistory || [],
        createdAt: doc.createdAt,
        updatedAt: doc.updatedAt,
        online: isOnline,
        lastHeartbeat: status?.lastHeartbeat || null,
        wifiRssi: status?.wifiRssi || null,
        ipAddress: status?.ipAddress || null,
        isDosing: status?.isDosing || false,
        totalDosedToday: status?.totalDosedToday || 0,
        uptimeSeconds: status?.uptimeSeconds || null,
        freeHeap: status?.freeHeap || null,
        lastSettingsSync: status?.lastSettingsSync || null,
        reportedEnabled: status?.reportedEnabled ?? null,
        reportedDailyVolume: status?.reportedDailyVolume ?? null,
        reportedScheduleSlots: status?.reportedScheduleSlots ?? null,
        reportedDayStartHour: status?.reportedDayStartHour ?? null,
        reportedDayEndHour: status?.reportedDayEndHour ?? null,
        reportedDayPercent: status?.reportedDayPercent ?? null,
        reportedStepsPerML: status?.reportedStepsPerML ?? null,
        reportedActiveProfile: status?.reportedActiveProfile ?? null,
        reportedPausedUntil: status?.reportedPausedUntil ?? null,
        settingsMatch: mismatches.length === 0,
        mismatches,
      };
    });
  }

  @Post()
  async upsertSettings(
    @Body() dto: UpdatePumpSettingsDto,
  ): Promise<{ success: boolean; message: string; pumpId: string }> {
    const result = await this.pumpSettingsService.upsertSettings(dto);

    // Queue SAVE_SETTINGS command so ESP32 saves new settings to EEPROM
    await this.pumpCommandsService.queueSaveSettings(result.pumpId);

    return {
      success: true,
      message: 'Settings saved, command queued for device sync',
      pumpId: result.pumpId,
    };
  }

  @Delete(':pumpId')
  async deleteSettings(
    @Param('pumpId') pumpId: string,
  ): Promise<{ success: boolean; message: string }> {
    const result = await this.pumpSettingsService.deleteSettings(pumpId);
    if (!result) {
      throw new NotFoundException(`Pump settings not found for pumpId: ${pumpId}`);
    }
    return {
      success: true,
      message: 'Settings deleted',
    };
  }

  @Post('sync-from-device/:pumpId')
  async syncFromDevice(
    @Param('pumpId') pumpId: string,
    @Body()       body: {
      enabled: boolean;
      dailyVolume: number;
      scheduleSlots: number;
      dayStartHour: number;
      dayEndHour: number;
      dayPercent: number;
      stepsPerML: number;
      activeProfile: number;
      pausedUntil: number;
    },
  ): Promise<{ success: boolean; message: string }> {
    await this.pumpSettingsService.upsertSettings({
      pumpId,
      ...body,
    });
    return {
      success: true,
      message: 'Server settings synced from device (EEPROM)',
    };
  }

  @Post('report/:pumpId')
  async reportSettings(
    @Param('pumpId') pumpId: string,
    @Body() dto: ReportPumpSettingsDto,
  ): Promise<{ success: boolean }> {
    // Save device-reported values for comparison (do NOT overwrite server settings)
    await this.pumpStatusService.reportStatus({
      pumpId,
      online: dto.online ?? true,
      lastHeartbeat: dto.lastHeartbeat ?? Math.floor(Date.now() / 1000),
      wifiRssi: dto.wifiRssi,
      ipAddress: dto.ipAddress,
      isDosing: dto.isDosing,
      totalDosedToday: dto.totalDosedToday,
      uptimeSeconds: dto.uptimeSeconds,
      freeHeap: dto.freeHeap,
      reportedEnabled: dto.enabled,
      reportedDailyVolume: dto.dailyVolume,
      reportedScheduleSlots: dto.scheduleSlots,
      reportedDayStartHour: dto.dayStartHour,
      reportedDayEndHour: dto.dayEndHour,
      reportedDayPercent: dto.dayPercent,
      reportedStepsPerML: dto.stepsPerML,
      reportedActiveProfile: dto.activeProfile,
      reportedPausedUntil: dto.pausedUntil,
      lastSettingsSync: dto.lastSettingsSync,
    } as any);

    return { success: true };
  }

  private findMismatches(server: Record<string, any>, reported: Record<string, any>): string[] {
    const mismatches: string[] = [];
    for (const key of Object.keys(server)) {
      const sVal = server[key];
      const rVal = reported[key];
      if (rVal !== undefined && rVal !== null && sVal !== rVal) {
        mismatches.push(`${key}: server=${sVal} vs device=${rVal}`);
      }
    }
    return mismatches;
  }
}
