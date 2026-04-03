import { Module } from '@nestjs/common';
import { EventEmitterModule } from '@nestjs/event-emitter';
import { SerialController } from './serial.controller';
import { SerialService } from './serial.service';

@Module({
  imports: [EventEmitterModule.forRoot()],
  controllers: [SerialController],
  providers: [SerialService],
})
export class SerialModule {}
