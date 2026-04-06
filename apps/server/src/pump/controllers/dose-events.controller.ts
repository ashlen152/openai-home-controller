import { Controller, Get, Post, Body, Param } from '@nestjs/common';
import { DoseEventsService } from '../services/dose-events.service';
import { CreateDoseEventDto } from '../dto/create-dose-event.dto';
import { DoseEvent, DoseEventStatus } from '../schemas/dose-event.schema';

@Controller('dose-events')
export class DoseEventsController {
  constructor(private readonly doseEventsService: DoseEventsService) {}

  @Post()
  async logDoseEvent(
    @Body() dto: CreateDoseEventDto,
  ): Promise<{ success: boolean; eventId: string; message: string }> {
    const result = await this.doseEventsService.logDoseEvent(dto);
    return {
      success: true,
      eventId: result.eventId,
      message: 'Dose event logged',
    };
  }

  @Get(':pumpId')
  async getDoseHistory(@Param('pumpId') pumpId: string): Promise<{
    pumpId: string;
    totalToday: number;
    doseCount: number;
    doses: {
      eventId: string;
      timestamp: number;
      dosingTimestamp: number | null;
      volume: number;
      status: string;
      success: boolean | null;
    }[];
  }> {
    const doses = await this.doseEventsService.getDoseHistory(pumpId);
    const completedDoses = doses.filter((d: DoseEvent) => d.status === DoseEventStatus.COMPLETED);
    return {
      pumpId,
      totalToday: completedDoses.reduce((sum: number, dose: DoseEvent) => sum + dose.volume, 0),
      doseCount: completedDoses.length,
      doses: completedDoses.map((dose: DoseEvent) => ({
        eventId: dose.eventId,
        timestamp: dose.timestamp,
        dosingTimestamp: dose.dosingTimestamp ?? null,
        volume: dose.volume,
        status: dose.status,
        success: dose.success ?? null,
      })),
    };
  }

  @Get(':pumpId/today')
  async getTodaysDoses(@Param('pumpId') pumpId: string): Promise<{
    pumpId: string;
    totalToday: number;
    doseCount: number;
    doses: {
      eventId: string;
      timestamp: number;
      dosingTimestamp: number | null;
      volume: number;
      status: string;
      success: boolean | null;
    }[];
  }> {
    const doses = await this.doseEventsService.getTodaysDoses(pumpId);
    const completedDoses = doses.filter((d: DoseEvent) => d.status === DoseEventStatus.COMPLETED);
    return {
      pumpId,
      totalToday: completedDoses.reduce((sum: number, dose: DoseEvent) => sum + dose.volume, 0),
      doseCount: completedDoses.length,
      doses: completedDoses.map((dose: DoseEvent) => ({
        eventId: dose.eventId,
        timestamp: dose.timestamp,
        dosingTimestamp: dose.dosingTimestamp ?? null,
        volume: dose.volume,
        status: dose.status,
        success: dose.success ?? null,
      })),
    };
  }
}
