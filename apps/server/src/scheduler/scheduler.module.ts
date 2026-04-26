import { Module } from '@nestjs/common';
import { BullModule } from '@nestjs/bullmq';
import { ScheduleModule } from '@nestjs/schedule';
import { DbModule } from '../db/db.module';
import { ChapterSchedulerService } from './chapter-scheduler.service';

@Module({
  imports: [DbModule, BullModule.registerQueue({ name: 'crawl-queue' }), ScheduleModule.forRoot()],
  providers: [ChapterSchedulerService],
  exports: [ChapterSchedulerService, BullModule],
})
export class SchedulerModule {}
