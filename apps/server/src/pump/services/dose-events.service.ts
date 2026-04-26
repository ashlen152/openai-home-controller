import { Injectable, BadRequestException } from '@nestjs/common';
import { InjectModel } from '@nestjs/mongoose';
import { Model } from 'mongoose';
import { DoseEvent, DoseEventDocument } from '../schemas/dose-event.schema';
import { CreateDoseEventDto } from '../dto/create-dose-event.dto';
import { DoseEventStatus } from '../schemas/dose-event.schema';

@Injectable()
export class DoseEventsService {
  constructor(@InjectModel(DoseEvent.name) private doseEventModel: Model<DoseEventDocument>) {
    // Initialize the background flush mechanism (for production) and test-time adjustments
    this.ensureFlusherInitialized();
  }

  // In-memory buffering for batch writes
  private batchBuffer: Array<{ dto: CreateDoseEventDto; pendingIndex: number }> = [];
  private pendingWrites: Array<{ resolve: (value: DoseEvent) => void; reject: (err: any) => void }> = [];
  private batchSize: number = 5; // batch when we have 5 events
  private flushIntervalMs: number = process.env.NODE_ENV === 'test' ? 0 : 5000; // 5s flush interval; test env flushes immediately
  private flushTimer?: any;
  private maxRetries: number = 3;

  private ensureFlusherInitialized() {
    if (!this.flushTimer && this.flushIntervalMs > 0) {
      this.flushTimer = setInterval(() => {
        this.flushBatch().catch(() => {
          // swallow; we'll retry on next interval
        });
      }, this.flushIntervalMs);
    }
  }

  // Enqueue a dose event into the batch and return a promise that resolves when the event is persisted
  async logDoseEvent(dto: CreateDoseEventDto): Promise<DoseEvent> {
    // Keep existing validation rules for consistency
    if (dto.status === DoseEventStatus.STARTED) {
      if (dto.success !== null) {
        throw new BadRequestException('For started events, success must be null');
      }
    } else if (dto.status === DoseEventStatus.COMPLETED || dto.status === DoseEventStatus.FAILED) {
      if (typeof dto.success !== 'boolean') {
        throw new BadRequestException('For completed/failed events, success must be boolean');
      }
    }

    return new Promise<DoseEvent>((resolve, reject) => {
      const pendingIndex = this.pendingWrites.length;
      this.pendingWrites.push({ resolve, reject });
      this.batchBuffer.push({ dto, pendingIndex });

      // Trigger an immediate flush if test environment or buffer reached threshold
      if (this.flushIntervalMs === 0 || this.batchBuffer.length >= this.batchSize) {
        this.flushBatch().catch(() => {
          // If batch flush fails here, error will be propagated to individual promises below
        });
      }
      this.ensureFlusherInitialized();
    });
  }

  // Flush the current batch to the database
  private async flushBatch(): Promise<void> {
    if (this.batchBuffer.length === 0) return;

    const currentBatch = this.batchBuffer;
    this.batchBuffer = [];

    for (const entry of currentBatch) {
      const { dto, pendingIndex } = entry;
      let attempts = 0;
      let succeeded = false;
      let lastError: any = null;
      // Simple retry loop with backoff
      while (!succeeded && attempts < this.maxRetries) {
        try {
          const created = await this.doseEventModel.create(dto);
          this.pendingWrites[pendingIndex]?.resolve?.(created as any);
          succeeded = true;
        } catch (err) {
          lastError = err;
          attempts++;
          if (attempts < this.maxRetries) {
            await new Promise(res => setTimeout(res, 100 * attempts));
          }
        }
      }
      if (!succeeded) {
        // If still failing after retries, reject to avoid leaking promises
        this.pendingWrites[pendingIndex]?.reject?.(lastError);
      }
    }
  }

  

  async getDoseHistory(pumpId: string): Promise<DoseEvent[]> {
    return this.doseEventModel.find({ pumpId }).sort({ timestamp: -1 }).exec();
  }

  async getTodaysDoses(pumpId: string): Promise<DoseEvent[]> {
    const startOfDay = new Date();
    startOfDay.setHours(0, 0, 0, 0);
    const endOfDay = new Date();
    endOfDay.setHours(23, 59, 59, 999);

    return this.doseEventModel
      .find({
        pumpId,
        timestamp: {
          $gte: startOfDay.getTime() / 1000,
          $lte: endOfDay.getTime() / 1000,
        },
      })
      .sort({ timestamp: -1 })
      .exec();
  }
}
