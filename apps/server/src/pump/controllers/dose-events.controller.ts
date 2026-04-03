import { Controller, Get, Post, Body, Param } from '@nestjs/common';
import { DoseEventsService } from '../services/dose-events.service';
import { CreateDoseEventDto } from '../dto/create-dose-event.dto';
import { DoseEvent } from '../schemas/dose-event.schema';

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
      volume: number;
      status: string;
      success: boolean | null;
    }[];
  }> {
    const doses = await this.doseEventsService.getDoseHistory(pumpId);
    return {
      pumpId,
      totalToday: doses.reduce((sum: number, dose: DoseEvent) => sum + dose.volume, 0),
      doseCount: doses.length,
      doses: doses.map((dose: DoseEvent) => ({
        eventId: dose.eventId,
        timestamp: dose.timestamp,
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
      volume: number;
      status: string;
      success: boolean | null;
    }[];
  }> {
    const doses = await this.doseEventsService.getTodaysDoses(pumpId);
    return {
      pumpId,
      totalToday: doses.reduce((sum: number, dose: DoseEvent) => sum + dose.volume, 0),
      doseCount: doses.length,
      doses: doses.map((dose: DoseEvent) => ({
        eventId: dose.eventId,
        timestamp: dose.timestamp,
        volume: dose.volume,
        status: dose.status,
        success: dose.success ?? null,
      })),
    };
  }
}
