import { INestApplication } from '@nestjs/common';

export async function setupCors(app: INestApplication) {
  app.enableCors();
}