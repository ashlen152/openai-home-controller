import { ValidationPipe } from '@nestjs/common';
import { INestApplication, Logger } from '@nestjs/common';

export async function setupValidation(app: INestApplication) {
  const logger = new Logger('Validation');

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
}