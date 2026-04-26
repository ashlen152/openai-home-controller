import { INestApplication } from '@nestjs/common';
import { SwaggerModule, DocumentBuilder } from '@nestjs/swagger';

export async function setupSwagger(app: INestApplication) {
  const config = new DocumentBuilder()
    .setTitle('OpenAI Workflow API')
    .setDescription('Modular RAG manga tracker API')
    .setVersion('1.0')
    .build();

  const document = SwaggerModule.createDocument(app, config);
  SwaggerModule.setup('api', app, document);
}