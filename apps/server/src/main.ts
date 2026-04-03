import { NestFactory } from '@nestjs/core';
import { ValidationPipe, Logger } from '@nestjs/common';
import { SwaggerModule, DocumentBuilder } from '@nestjs/swagger';
import { NestExpressApplication } from '@nestjs/platform-express';
import { join } from 'path';
import * as os from 'os';
import { AppModule } from './app.module';
import { GlobalExceptionFilter } from './common/filters/global-exception.filter';
import { LoggingInterceptor } from './common/interceptors/logging.interceptor';

async function bootstrap() {
  const app = await NestFactory.create<NestExpressApplication>(AppModule);
  const logger = new Logger('Bootstrap');

  app.enableCors();
  app.useStaticAssets(join(__dirname, '..', '..', 'public'));
  app.setGlobalPrefix('api');

  app.useGlobalFilters(new GlobalExceptionFilter());
  app.useGlobalInterceptors(new LoggingInterceptor());

  app.useGlobalPipes(
    new ValidationPipe({
      whitelist: true,
      transform: true,
      forbidNonWhitelisted: true,
      exceptionFactory: (errors) => {
        const messages = errors.map((e) => ({
          field: e.property,
          constraints: e.constraints,
          value: e.value,
        }));
        logger.error(`Validation failed: ${JSON.stringify(messages)}`);
        const { BadRequestException } = require('@nestjs/common');
        return new BadRequestException({
          message: 'Validation failed',
          errors: messages,
        });
      },
    }),
  );

  const config = new DocumentBuilder()
    .setTitle('OpenAI Workflow API')
    .setDescription('Modular RAG manga tracker API')
    .setVersion('1.0')
    .build();
  const document = SwaggerModule.createDocument(app, config);
  SwaggerModule.setup('api', app, document);

  const port = 3000;
  await app.listen(port, '0.0.0.0');

  const server = app.getHttpServer();
  const address = server.address();
  const ip = typeof address === 'object' && address ? address.address : '0.0.0.0';
  const host = ip === '0.0.0.0' ? 'localhost' : ip;

  console.log(`Application running on: http://${host}:${port}`);

  if (process.argv.includes('--mdns')) {
    const { exec } = require('child_process');
    const ifaces = os.networkInterfaces();
    let targetIp = '127.0.0.1';
    for (const name of Object.keys(ifaces)) {
      for (const iface of ifaces[name] || []) {
        // Prefer en1 (Wi-Fi), skip Docker interfaces
        if (iface.family === 'IPv4' && !iface.internal && !name.includes('docker')) {
          targetIp = iface.address;
          if (name === 'en1') break; // Prefer Wi-Fi
        }
      }
      if (targetIp !== '127.0.0.1') break;
    }
    console.log(`Registering mDNS with IP: ${targetIp}`);
    exec(`dns-sd -P openai _http._tcp local ${port} openai.local. ${targetIp} &`);
    console.log(`mDNS advertised: http://openai.local:${port}`);
  }
}
bootstrap();
