import { NestFactory } from '@nestjs/core';
import { Logger } from '@nestjs/common';
import { NestExpressApplication } from '@nestjs/platform-express';
import { join } from 'path';
import { AppModule } from './app.module';
import { setupCors } from './config/setup-cors';
import { setupValidation } from './config/setup-validation';
import { setupFilters } from './config/setup-filters';
import { setupSwagger } from './config/setup-swagger';
import { setupMdns } from './config/setup-mdns';

async function bootstrap() {
  const app = await NestFactory.create<NestExpressApplication>(AppModule);
  const logger = new Logger('Bootstrap');

  app.useStaticAssets(join(__dirname, '..', '..', 'public'));
  app.setGlobalPrefix('api');

  await setupCors(app);
  await setupValidation(app);
  await setupFilters(app);
  await setupSwagger(app);

  const port = 3000;
  await app.listen(port, '0.0.0.0');

  const server = app.getHttpServer();
  const address = server.address();
  const ip = typeof address === 'object' && address ? address.address : '0.0.0.0';
  const host = ip === '0.0.0.0' ? 'localhost' : ip;

  console.log(`Application running on: http://${host}:${port}`);

  await setupMdns(app, port);
}
bootstrap();