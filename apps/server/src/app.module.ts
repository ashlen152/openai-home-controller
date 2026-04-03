import { Module } from '@nestjs/common';
import { ConfigModule } from '@nestjs/config';
import { MongooseModule } from '@nestjs/mongoose';
import { BullModule } from '@nestjs/bullmq';
import configuration from './config/configuration';
import { AdaptersModule } from './adapters/adapters.module';
import { CrawlerModule } from './crawler/crawler.module';
import { AuthModule } from './auth/auth.module';
import { DbModule } from './db/db.module';
import { RagModule } from './rag/rag.module';
import { WorkflowsModule } from './workflows/workflows.module';
import { SchedulerModule } from './scheduler/scheduler.module';
import { MangasModule } from './mangas/mangas.module';
import { HealthModule } from './health/health.module';
import { PumpModule } from './pump/pump.module';
import { LogsModule } from './logs/logs.module';
import { SerialModule } from './serial/serial.module';

@Module({
  imports: [
    ConfigModule.forRoot({ isGlobal: true, load: [configuration] }),
    MongooseModule.forRoot(process.env.MONGODB_URI || 'mongodb://localhost:27017/openai-workflow'),
    BullModule.forRoot({
      connection: {
        host: process.env.REDIS_HOST || 'localhost',
        port: parseInt(process.env.REDIS_PORT || '6379', 10),
      },
    }),
    AuthModule,
    AdaptersModule,
    CrawlerModule,
    DbModule,
    RagModule,
    WorkflowsModule,
    SchedulerModule,
    MangasModule,
    HealthModule,
    PumpModule,
    LogsModule,
    SerialModule,
  ],
})
export class AppModule {}
