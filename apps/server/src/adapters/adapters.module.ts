import { Module } from '@nestjs/common';
import { AsuraScansAdapter } from './asurascans/asurascans.adapter';
import { MANGO_ADAPTER } from './interfaces/manga-adapter.interface';
import { AuthModule } from '../auth/auth.module';

@Module({
  imports: [AuthModule],
  providers: [
    AsuraScansAdapter,
    { provide: MANGO_ADAPTER, useExisting: AsuraScansAdapter },
  ],
  exports: [AsuraScansAdapter, MANGO_ADAPTER],
})
export class AdaptersModule {}
